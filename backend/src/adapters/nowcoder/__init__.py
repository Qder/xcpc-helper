"""牛客竞赛（NowCoder）适配器。

数据源：practice-coding 分页 HTML（服务端渲染）+ rating-history JSON API（绑定验证）。
详见 docs/design/activity/nowcoder.md。
"""

import logging
import re
from collections.abc import AsyncIterator
from datetime import datetime, timedelta, timezone
from typing import Any

from adapters.base import (
    AuthMode,
    Capability,
    Credentials,
    PlatformAdapter,
    PlatformError,
    PlatformSubmission,
    ProblemMeta,
    ProgressCallback,
    SyncBatch,
    TagMeta,
    UserInfo,
    UserNotFoundError,
)
from adapters.net import HttpFetcher
from adapters.nowcoder.api_models import (
    NcKnowledgeCatalogEnvelope,
    NcProblemListEnvelope,
    NcRatingHistoryEnvelope,
    NcSubmissionRow,
)
from adapters.nowcoder.normalize import (
    extract_problem_id_from_page,
    is_practice_url,
    map_verdict,
    parse_problem_url,
)

logger = logging.getLogger("xcpc.adapters.nowcoder")

BASE = "https://ac.nowcoder.com"
PRACTICE_CODING_URL = f"{BASE}/acm/contest/profile/{{uid}}/practice-coding"
RATING_HISTORY_URL = f"{BASE}/acm/contest/rating-history"
PROBLEM_LIST_JSON_URL = f"{BASE}/acm/problem/list/json"
KNOWLEDGE_CATALOG_URL = f"{BASE}/acm/problem/knowledge/catalog"

PAGE_SIZE = 50  # 经实测，200 有重复页 bug，50 稳定
MAX_PAGES = 100  # 安全护栏
CHINA_TZ = timezone(timedelta(hours=8))  # 中国时区 UTC+8

# 题库列表页每页条数（与提交页不同：题库页 pageSize=200 返回空表，必须用默认 50）
PROBLEM_PAGE_SIZE = 50
# 推荐候选的难度窗口（相对源题）：略低到略高，与 review.RATING_LOW/HIGH 同口径
PROBLEM_RATING_LOW = 200
PROBLEM_RATING_HIGH = 100

# 个人主页 HTML 提取正则（提取用户名/昵称）
_CODER_NAME_RE = re.compile(
    r'<a[^>]*class="coder-name[^"]*"[^>]*>([^<]*)</a>'
)
_TITLE_NAME_RE = re.compile(r'<title>([^<]*)的比赛主页</title>')
_ROW_RE = re.compile(r"<tr[^>]*>(.*?)</tr>", re.DOTALL)
_TD_RE = re.compile(r"<td[^>]*>(.*?)</td>", re.DOTALL)
_SID_RE = re.compile(r"submissionId=(\d+)")
_PROB_RE = re.compile(r'href="/acm/problem/([^"]+)"[^>]*>([^<]*)</a>')
_STATUS_RE = re.compile(r">([^<]*)</a>")
_TAG_RE = re.compile(r"<[^>]+>")

# ===== 题库列表页（PROBLEM_LOOKUP）正则 =====
# 题库列表行以 data-problemId 标识（与提交页的表格结构不同）
_PROBLEM_ROW_RE = re.compile(r'<tr data-problemId="(\d+)"[^>]*>(.*?)</tr>', re.DOTALL)
# 题目标题：<a href="/acm/problem/325377" class="title">小月的长廊</a>
_PROBLEM_TITLE_RE = re.compile(r'class="title">([^<]*)</a>')
# 知识点标签：<a class="tag-label js-tag" data-id="1286">差分/前缀和</a>
_PROBLEM_TAG_RE = re.compile(
    r'class="tag-label[^"]*"[^>]*data-id="(\d+)"[^>]*>([^<]*)</a>'
)
_PROBLEM_LIST_URL = f"{BASE}/acm/problem/list"


class NowcoderAdapter(PlatformAdapter):
    platform_id = "nowcoder"
    name = "牛客竞赛"
    capabilities = frozenset(
        {Capability.SUBMISSIONS, Capability.USER_INFO, Capability.PROBLEM_LOOKUP}
    )
    auth = AuthMode.NONE
    min_interval = 1.0  # 保守限流
    homepage_url = "https://ac.nowcoder.com"

    def __init__(self, fetcher: HttpFetcher) -> None:
        self._fetcher = fetcher

    # ===== 绑定验证 =====

    async def verify(
        self, handle: str, credentials: Credentials | None = None
    ) -> UserInfo:
        """验证用户存在性并获取展示名。

        1. rating-history API 判存在性（data 非空即存在）；
        2. 个人主页 HTML 提取用户名（coder-name 标签或 title）。
        """
        # 步骤 1：存在性验证
        data = await self._fetcher.get_json(
            RATING_HISTORY_URL,
            params={"uid": handle},
            platform=self.platform_id,
            min_interval=self.min_interval,
        )
        try:
            envelope = NcRatingHistoryEnvelope.model_validate(data)
        except Exception as exc:
            raise PlatformError(f"牛客 rating-history 响应格式异常: {exc}") from exc
        if envelope.code != 0:
            raise PlatformError(f"牛客返回错误 code={envelope.code}")
        if not envelope.data:
            raise UserNotFoundError(f"牛客用户不存在: {handle}")

        # 步骤 2：获取展示名（用户名）
        display_name = await self._fetch_display_name(handle)
        return UserInfo(handle=handle, display_name=display_name)

    async def _fetch_display_name(self, handle: str) -> str | None:
        """从个人主页 HTML 提取用户名/昵称。"""
        try:
            url = f"{BASE}/acm/contest/profile/{handle}"
            resp = await self._fetcher.request(
                "GET", url, platform=self.platform_id, min_interval=self.min_interval
            )
            html = resp.text

            # 优先：coder-name 标签（如 <a class="coder-name rate-score7">UESTC_Vici</a>）
            m = _CODER_NAME_RE.search(html)
            if m:
                name = m.group(1).strip()
                if name:
                    return name

            #  fallback：title 标签（如 <title>UESTC_Vici的比赛主页</title>）
            m = _TITLE_NAME_RE.search(html)
            if m:
                name = m.group(1).strip()
                if name and name != "比赛主页":
                    return name
        except Exception as exc:  # noqa: BLE001
            logger.debug("牛客获取用户名失败: %s", exc)
        return None

    # ===== 提交拉取 =====

    async def fetch_submissions(
        self,
        handle: str,
        *,
        since: int | None,
        credentials: Credentials | None = None,
        full_window_days: int,
        full_min_rows: int,
        progress_cb: ProgressCallback | None = None,
        resume_checkpoint: dict[str, Any] | None = None,
    ) -> AsyncIterator[SyncBatch]:
        """按页流式拉取提交（每页一批，按时间倒序）。

        牛客无总量字段，不上报进度（progress_cb 为契约参数，本平台忽略）。

        - 增量（since 非空）：遇 ts < since 即停；游标当秒提交重复拉取，
          由 store 层按 submission_id 去重吸收；
        - 全量（since 为空）：拉到覆盖 full_window_days 窗口为止，窗口内
          不足 full_min_rows 条时继续拉满；断点 = {"page": 下一页页码,
          "fetched": 累计条数}；
        - 绝对护栏：最多 MAX_PAGES 页。
        """
        page = 1
        fetched = 0
        if since is None and resume_checkpoint:
            page = int(resume_checkpoint.get("page", 1))
            fetched = int(resume_checkpoint.get("fetched", 0))

        for _ in range(page, MAX_PAGES + 1):
            html = await self._fetch_html(handle, page)
            rows = self._parse_rows(html)
            if not rows:
                yield SyncBatch(done=True)
                return

            batch: list[PlatformSubmission] = []
            hit_old = False
            for row in rows:
                ts = row.submitted_at_str
                ts_utc = self.to_utc_seconds(ts)
                if since is not None and ts_utc < since:
                    hit_old = True
                    break
                batch.append(self.to_submission(row, ts_utc))

            fetched += len(batch)
            done = hit_old or len(rows) < PAGE_SIZE
            yield SyncBatch(
                items=batch,
                checkpoint=(
                    None
                    if done or since is not None
                    else {"page": page + 1, "fetched": fetched}
                ),
                done=done,
            )
            if done:
                return
            page += 1

        yield SyncBatch(done=True)

    # ===== 题目查询（PROBLEM_LOOKUP，错题本同类推荐消费） =====

    async def fetch_problem(self, problem_key: str) -> ProblemMeta | None:
        """按题号查单题画像（题库列表页 keyword 精确查询）。

        牛客题库无法一次拉全站（约 300 页），改用定点查询：keyword 传题号
        会精确返回单条，含知识点标签与难度。查不到返回 None。
        """
        rows = await self._fetch_problem_rows(keyword=problem_key)
        for meta in rows:
            if meta.problem_key == problem_key:
                return meta
        return None

    async def find_similar_problems(
        self, target: ProblemMeta, *, limit: int
    ) -> list[ProblemMeta]:
        """按源题首个标签 + 难度窗口查候选（服务端过滤，减少请求与候选规模）。

        牛客的 tagId 过滤只认主键，故用 target.tag_ids；没有主键标签时退回
        不限标签的难度窗口查询（候选靠上层按标签文本过滤）。
        只取一页即可覆盖打分所需（同标签同难度段的题目量远小于一页）。
        """
        params: dict[str, Any] = {"pageSize": PROBLEM_PAGE_SIZE, "page": 1}
        if target.tag_ids:
            # 实测：多个 tagId 以逗号分隔是 OR 语义，候选更全，交给上层按标签交集排序
            params["tagId"] = ",".join(target.tag_ids)
        if target.rating is not None:
            params["minDifficulty"] = max(0, target.rating - PROBLEM_RATING_LOW)
            params["maxDifficulty"] = target.rating + PROBLEM_RATING_HIGH

        rows = await self._fetch_problem_rows(params=params)
        # 上层还要按共同标签数排序、排除已 AC 与源题，这里多留一些余量
        return rows[: max(limit * 8, PROBLEM_PAGE_SIZE)]

    async def _fetch_problem_rows(
        self,
        *,
        keyword: str | None = None,
        params: dict[str, Any] | None = None,
    ) -> list[ProblemMeta]:
        """拉取题库列表页并解析为题目画像（单页）。"""
        query: dict[str, Any] = {"pageSize": PROBLEM_PAGE_SIZE, "page": 1}
        if keyword is not None:
            query["keyword"] = keyword
        if params:
            query.update(params)

        resp = await self._fetcher.request(
            "GET",
            _PROBLEM_LIST_URL,
            params=query,
            platform=self.platform_id,
            min_interval=self.min_interval,
        )
        return self._parse_problem_rows(resp.text)

    @staticmethod
    def _parse_problem_rows(html: str) -> list[ProblemMeta]:
        """解析题库列表页 HTML 为题目画像列表。

        每行结构：题号单元格 + 标题/标签单元格 + 难度 + 通过人数。
        难度列为空时（部分旧题未定级）rating 置 None；缺标题的行跳过。
        """
        out: list[ProblemMeta] = []
        for pid, body in _PROBLEM_ROW_RE.findall(html):
            title_match = _PROBLEM_TITLE_RE.search(body)
            if not title_match:
                continue
            name = title_match.group(1).strip()
            tags: list[str] = []
            tag_ids: list[str] = []
            for tag_id, tag_text in _PROBLEM_TAG_RE.findall(body):
                tag_ids.append(tag_id)
                tags.append(tag_text.strip())
            cells = [
                _TAG_RE.sub("", td).strip() for td in _TD_RE.findall(body)
            ]
            # 列序：题号 / 标题+标签 / 难度 / 通过人数；难度解析失败视为未定级
            rating: int | None = None
            if len(cells) >= 3 and cells[2].isdigit():
                rating = int(cells[2])
            out.append(
                ProblemMeta(
                    problem_key=pid,
                    name=name,
                    rating=rating,
                    tags=tags,
                    tag_ids=tag_ids,
                    url=f"{BASE}/acm/problem/{pid}",
                )
            )
        return out

    # ===== 题目链接解析（手动收录） =====

    @staticmethod
    def parse_problem_url(url: str) -> str | None:
        """题目链接 → problem_key（纯数字题号）；纯字符串解析。"""
        return parse_problem_url(url)

    async def resolve_problem_url(self, url: str) -> str | None:
        """链接 → 题号；题霸链接（/practice/{uuid}）需抓页面取 problemId。

        纯函数能解析的（/acm/problem/{id}、比赛题）不发请求；只有题霸链接
        会多一次页面抓取。抓取失败返回 None，由调用方给出「无法识别」提示。
        """
        key = parse_problem_url(url)
        if key is not None:
            return key
        if not is_practice_url(url):
            return None
        try:
            resp = await self._fetcher.request(
                "GET",
                url,
                platform=self.platform_id,
                min_interval=self.min_interval,
            )
        except PlatformError as exc:
            logger.warning("牛客题霸页面抓取失败：%s", exc)
            return None
        return extract_problem_id_from_page(resp.text)

    # ===== 按算法查找（标签目录 + 按标签搜题） =====

    async def list_problem_tags(self) -> list[TagMeta]:
        """知识点目录（knowledge/catalog，裸 JSON）。

        注意这是「按知识点练习」的精选目录（15 组 155 个叶子），不是题库 tag
        的全集；但目录里的 id 可以直接当题库的 tagId 用（实测一致）。
        totalCount 是该知识点下的题目数，可直接展示。
        """
        data = await self._get_json(
            KNOWLEDGE_CATALOG_URL, NcKnowledgeCatalogEnvelope, "知识点目录"
        )
        if data is None or data.data is None:
            return []
        out: list[TagMeta] = []
        for group in data.data.groups:
            if group.hidden:
                continue
            for child in group.childTags:
                out.append(
                    TagMeta(
                        id=str(child.id),
                        name=child.name or str(child.id),
                        group=child.groupName or group.name,
                        count=child.totalCount,
                    )
                )
        return out

    async def search_problems(
        self, tag_id: str, *, page: int = 1
    ) -> list[ProblemMeta]:
        """按知识点浏览题库一页（list/json，裸 JSON，比 HTML 稳）。"""
        data = await self._get_json(
            PROBLEM_LIST_JSON_URL,
            NcProblemListEnvelope,
            "题库列表",
            params={"tagId": tag_id, "page": page, "pageSize": PROBLEM_PAGE_SIZE},
        )
        if data is None or data.data is None:
            return []
        return [self._to_problem_meta(row) for row in data.data.problemSets]

    @staticmethod
    def _to_problem_meta(row: Any) -> ProblemMeta:
        """JSON 题目行 → ProblemMeta（key 用纯数字 problemId，不用 showId）。"""
        return ProblemMeta(
            problem_key=str(row.problemId),
            name=row.name,
            rating=row.difficulty,
            tags=[t.name for t in row.tagList],
            tag_ids=[str(t.id) for t in row.tagList],
            url=f"{BASE}/acm/problem/{row.problemId}",
        )

    async def _get_json(
        self,
        url: str,
        model: Any,
        label: str,
        *,
        params: dict[str, Any] | None = None,
    ) -> Any:
        """GET 裸 JSON 并校验信封；code != 0 或解析失败抛 PlatformError。"""
        resp = await self._fetcher.request(
            "GET",
            url,
            params=params,
            platform=self.platform_id,
            min_interval=self.min_interval,
        )
        try:
            envelope = model.model_validate_json(resp.text)
        except ValueError as exc:
            raise PlatformError(f"牛客 {label}格式异常: {exc}") from exc
        if envelope.code != 0:
            raise PlatformError(f"牛客 {label}返回错误: {envelope.msg}")
        return envelope

    # ===== 内部：HTTP =====

    async def _fetch_html(self, handle: str, page: int) -> str:
        """获取 practice-coding 页面 HTML。"""
        url = PRACTICE_CODING_URL.format(uid=handle)
        resp = await self._fetcher.request(
            "GET",
            url,
            params={
                "pageSize": PAGE_SIZE,
                "statusTypeFilter": -1,
                "languageCategoryFilter": -1,
                "orderType": "DESC",
                "page": page,
            },
            platform=self.platform_id,
            min_interval=self.min_interval,
        )
        return resp.text

    # ===== 内部：解析 =====

    @staticmethod
    def _parse_rows(html: str) -> list[NcSubmissionRow]:
        """从 HTML 中提取提交行列表。"""
        rows: list[NcSubmissionRow] = []
        raw_rows = _ROW_RE.findall(html)
        # 跳过表头行（第一个 <tr>）
        for raw in raw_rows[1:]:
            tds = _TD_RE.findall(raw)
            if len(tds) < 9:
                continue
            # 提交 ID
            sid_match = _SID_RE.search(tds[0])
            if not sid_match:
                continue
            submission_id = sid_match.group(1)
            # 题目
            prob_match = _PROB_RE.search(tds[1])
            if not prob_match:
                continue
            problem_id = prob_match.group(1)
            problem_name = prob_match.group(2).strip()
            # 状态
            status_match = _STATUS_RE.search(tds[2])
            status_text = status_match.group(1).strip() if status_match else ""
            # 语言
            language = _TAG_RE.sub("", tds[7]).strip()
            # 时间
            timestamp = _TAG_RE.sub("", tds[8]).strip()
            try:
                row = NcSubmissionRow(
                    submission_id=submission_id,
                    problem_id=problem_id,
                    problem_name=problem_name,
                    status_text=status_text,
                    language=language,
                    submitted_at_str=timestamp,
                )
            except Exception as exc:  # noqa: BLE001 - 单行解析失败跳过，不阻断整批
                logger.debug("牛客 HTML 单行解析失败: %s", exc)
                continue
            rows.append(row)
        return rows

    @staticmethod
    def to_utc_seconds(ts_str: str) -> int:
        """中国时区时间字符串 → UTC 秒级时间戳。"""
        dt = datetime.strptime(ts_str, "%Y-%m-%d %H:%M:%S").replace(tzinfo=CHINA_TZ)
        return int(dt.timestamp())

    @staticmethod
    def to_submission(row: NcSubmissionRow, ts_utc: int) -> PlatformSubmission:
        return PlatformSubmission(
            submission_id=row.submission_id,
            problem_key=row.problem_id,
            problem_name=row.problem_name,
            problem_url=f"https://ac.nowcoder.com/acm/problem/{row.problem_id}",
            difficulty=None,  # 牛客无难度信息
            verdict=map_verdict(row.status_text),
            submitted_at=ts_utc,
            language=row.language,
        )
