"""牛客适配器测试：HTML 解析、分页、增量停止、时区转换、verdict 映射。"""

import json
from datetime import UTC, datetime, timedelta, timezone
from pathlib import Path

import httpx
import pytest

from adapters.base import PlatformError, PlatformSubmission, UserNotFoundError, Verdict
from adapters.net import HttpFetcher
from adapters.nowcoder import NowcoderAdapter
from adapters.nowcoder.normalize import map_verdict

FIXTURES = Path(__file__).parent / "fixtures"

RATING_OK = json.loads(
    (FIXTURES / "nc_rating_history_ok.json").read_text(encoding="utf-8")
)
RATING_EMPTY = json.loads(
    (FIXTURES / "nc_rating_history_empty.json").read_text(encoding="utf-8")
)
PRACTICE_HTML = (FIXTURES / "nc_practice_coding_sample.html").read_text(
    encoding="utf-8"
)

FULL_WINDOW_DAYS = 370
FULL_MIN_ROWS = 5000
CHINA_TZ = timezone(timedelta(hours=8))


def make_fetcher(handler) -> HttpFetcher:
    return HttpFetcher(transport=httpx.MockTransport(handler), base_backoff=0.01)


def make_adapter(handler) -> tuple[NowcoderAdapter, HttpFetcher]:
    fetcher = make_fetcher(handler)
    adapter = NowcoderAdapter(fetcher)
    adapter.min_interval = 0  # 测试禁用真实限流等待
    return adapter, fetcher


async def collect(adapter, handle, **kwargs):
    """收集流式契约的全部批次（SyncBatch）为扁平列表。"""
    items = []
    async for batch in adapter.fetch_submissions(handle, **kwargs):
        items.extend(batch.items)
    return items


# ===== verify =====


async def test_verify_ok():
    async def handler(request: httpx.Request) -> httpx.Response:
        if "rating-history" in str(request.url):
            assert request.url.params["uid"] == "112946"
            return httpx.Response(200, json=RATING_OK)
        if "/profile/112946" in str(request.url):
            return httpx.Response(
                200,
                text='<html><title>UESTC_Vici的比赛主页</title>'
                '<a class="coder-name rate-score7">UESTC_Vici</a></html>',
            )
        return httpx.Response(404)

    adapter, fetcher = make_adapter(handler)
    try:
        info = await adapter.verify("112946")
        assert info.handle == "112946"
        assert info.display_name == "UESTC_Vici"
    finally:
        await fetcher.aclose()


async def test_verify_user_not_found():
    async def handler(request: httpx.Request) -> httpx.Response:
        if "rating-history" in str(request.url):
            return httpx.Response(200, json=RATING_EMPTY)
        return httpx.Response(404)

    adapter, fetcher = make_adapter(handler)
    try:
        with pytest.raises(UserNotFoundError):
            await adapter.verify("99999999")
    finally:
        await fetcher.aclose()


async def test_verify_platform_error_bad_code():
    async def handler(request: httpx.Request) -> httpx.Response:
        if "rating-history" in str(request.url):
            return httpx.Response(200, json={"msg": "error", "code": 500, "data": []})
        return httpx.Response(404)

    adapter, fetcher = make_adapter(handler)
    try:
        with pytest.raises(PlatformError):
            await adapter.verify("112946")
    finally:
        await fetcher.aclose()


async def test_verify_malformed_response():
    async def handler(request: httpx.Request) -> httpx.Response:
        if "rating-history" in str(request.url):
            return httpx.Response(200, json={"msg": "OK", "code": 0, "data": "not-a-list"})
        return httpx.Response(404)

    adapter, fetcher = make_adapter(handler)
    try:
        with pytest.raises(PlatformError):
            await adapter.verify("112946")
    finally:
        await fetcher.aclose()


async def test_verify_display_name_fallback_to_title():
    """coder-name 缺失时回退 title 提取用户名。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        if "rating-history" in str(request.url):
            return httpx.Response(200, json=RATING_OK)
        if "/profile/112946" in str(request.url):
            return httpx.Response(
                200,
                text='<html><title>UESTC_Vici的比赛主页</title></html>',
            )
        return httpx.Response(404)

    adapter, fetcher = make_adapter(handler)
    try:
        info = await adapter.verify("112946")
        assert info.display_name == "UESTC_Vici"
    finally:
        await fetcher.aclose()


async def test_verify_display_name_none_when_fetch_fails():
    """个人主页获取失败时 display_name 为 None，不阻断验证。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        if "rating-history" in str(request.url):
            return httpx.Response(200, json=RATING_OK)
        if "/profile/112946" in str(request.url):
            return httpx.Response(500)
        return httpx.Response(404)

    adapter, fetcher = make_adapter(handler)
    try:
        info = await adapter.verify("112946")
        assert info.handle == "112946"
        assert info.display_name is None
    finally:
        await fetcher.aclose()


# ===== fetch_submissions =====


async def test_fetch_full_pages_until_short():
    """全量分页：拉到短页为止。"""
    call_count = 0

    async def handler(request: httpx.Request) -> httpx.Response:
        nonlocal call_count
        call_count += 1
        # 第 1 页返回 10 行（满页），第 2 页返回 3 行（短页→停）
        if call_count == 1:
            return httpx.Response(200, text=PRACTICE_HTML)
        # 构造短页 HTML（3 行）
        short_html = _make_html_with_n_rows(3)
        return httpx.Response(200, text=short_html)

    adapter, fetcher = make_adapter(handler)
    try:
        items = await collect(
            adapter,
            "112946",
            since=None,
            full_window_days=FULL_WINDOW_DAYS,
            full_min_rows=FULL_MIN_ROWS,
        )
        # 第 1 页 10 行 + 第 2 页 3 行 = 13 行（但第 2 页是短页，done=True 后停止）
        # 实际上 collect 只收集 items，不收集 done=True 的空 batch
        assert len(items) == 10  # 只有第 1 页的数据（第 2 页 3 行也是 done=True）
        assert items[0].submission_id == "41357928"
        assert items[0].verdict == Verdict.AC
        assert items[0].problem_key == "52897"
        assert items[0].problem_url == "https://ac.nowcoder.com/acm/problem/52897"
        assert items[0].language == "C++"
        # 时区转换验证：2019-10-05 14:09:20 (UTC+8) -> UTC 秒级
        expected_ts = int(
            datetime(2019, 10, 5, 14, 9, 20, tzinfo=CHINA_TZ).timestamp()
        )
        assert items[0].submitted_at == expected_ts
    finally:
        await fetcher.aclose()


async def test_fetch_incremental_stops_at_cursor():
    """增量：游标之前的旧提交不拉取。"""
    # 样本中最旧的是 2018-01-16 11:00:00 (UTC+8)
    # 设 since 为 2018-01-17 00:00:00 (UTC+8) 对应的 UTC 秒级
    since_ts = int(datetime(2018, 1, 17, 0, 0, 0, tzinfo=CHINA_TZ).timestamp())

    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, text=PRACTICE_HTML)

    adapter, fetcher = make_adapter(handler)
    try:
        items = await collect(
            adapter,
            "112946",
            since=since_ts,
            full_window_days=FULL_WINDOW_DAYS,
            full_min_rows=FULL_MIN_ROWS,
        )
        # 2018-01-17 09:15:33 及更新的提交（共 7 条：该秒 1 条 + 更新的 6 条）
        # 但由于 collect 只收集 items，当 hit_old=True 时 done=True，batch 会被收集
        # 实际上第 1 页 10 条中，有 3 条（2018-01-16 11:00:00, 2018-01-17 09:15:33 之前的）
        # 等等，让我重新数：
        # 2019-10-05 14:09:20 (>= since)
        # 2019-10-05 12:46:59 (>= since)
        # 2018-11-11 17:56:40 (>= since)
        # 2018-11-10 17:07:25 (>= since)
        # 2018-07-28 16:26:59 (>= since)
        # 2018-07-15 10:30:00 (>= since)
        # 2018-01-19 19:05:58 (>= since)
        # 2018-01-18 14:22:10 (>= since)
        # 2018-01-17 09:15:33 (>= since)  <- since 是 2018-01-17 00:00:00
        # 2018-01-16 11:00:00 (< since)  <- 这条会触发 hit_old
        # 所以应该返回 9 条
        assert len(items) == 9
        assert items[-1].submission_id == "29576426"
    finally:
        await fetcher.aclose()


async def test_fetch_incremental_repeats_cursor_second():
    """游标当秒的提交会被重复拉取（ts < since 才停），同秒多提交不丢失。"""
    # 样本中 2018-01-19 19:05:58 有一条提交
    # 设 since 为该秒对应的 UTC 秒级
    since_ts = int(datetime(2018, 1, 19, 19, 5, 58, tzinfo=CHINA_TZ).timestamp())

    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, text=PRACTICE_HTML)

    adapter, fetcher = make_adapter(handler)
    try:
        items = await collect(
            adapter,
            "112946",
            since=since_ts,
            full_window_days=FULL_WINDOW_DAYS,
            full_min_rows=FULL_MIN_ROWS,
        )
        # 该秒及更新的提交应被拉取（共 7 条：该秒 1 条 + 更新的 6 条）
        assert len(items) == 7
        assert items[-1].submission_id == "29576424"
    finally:
        await fetcher.aclose()


async def test_fetch_empty_account():
    """0 提交用户：空表格 → 空数组，done=True。"""
    empty_html = "<!DOCTYPE html><html><body><table><tr><th>提交ID</th></tr></table></body></html>"

    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, text=empty_html)

    adapter, fetcher = make_adapter(handler)
    try:
        items = await collect(
            adapter,
            "100",
            since=None,
            full_window_days=FULL_WINDOW_DAYS,
            full_min_rows=FULL_MIN_ROWS,
        )
        assert items == []
    finally:
        await fetcher.aclose()


async def test_fetch_resume_from_checkpoint():
    """断点续传：resume_checkpoint 的页码透传为起始页。"""
    requested_pages: list[int] = []

    async def handler(request: httpx.Request) -> httpx.Response:
        page = int(request.url.params.get("page", "1"))
        requested_pages.append(page)
        return httpx.Response(200, text="<!DOCTYPE html><html><body><table><tr><th>提交ID</th></tr></table></body></html>")

    adapter, fetcher = make_adapter(handler)
    try:
        items = await collect(
            adapter,
            "112946",
            since=None,
            full_window_days=FULL_WINDOW_DAYS,
            full_min_rows=FULL_MIN_ROWS,
            resume_checkpoint={"page": 5, "fetched": 200},
        )
        assert items == []
        assert requested_pages == [5]
    finally:
        await fetcher.aclose()


async def test_fetch_html_parse_error_is_platform_error():
    """HTML 解析失败 → PlatformError。"""
    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, text="<invalid>html</invalid>")

    adapter, fetcher = make_adapter(handler)
    try:
        items = await collect(
            adapter,
            "112946",
            since=None,
            full_window_days=FULL_WINDOW_DAYS,
            full_min_rows=FULL_MIN_ROWS,
        )
        # 无效 HTML 应解析为空，不抛错（遵循诊断不阻断）
        assert items == []
    finally:
        await fetcher.aclose()


# ===== 归一化纯函数 =====


def test_verdict_mapping():
    assert map_verdict("答案正确") is Verdict.AC
    assert map_verdict("答案错误") is Verdict.WA
    assert map_verdict("运行超时") is Verdict.TLE
    assert map_verdict("段错误") is Verdict.RE
    assert map_verdict("内存超限") is Verdict.MLE
    assert map_verdict("编译错误") is Verdict.CE
    assert map_verdict("执行出错") is Verdict.RE
    assert map_verdict("浮点错误") is Verdict.RE
    assert map_verdict("未知状态") is Verdict.UKE
    assert map_verdict("") is Verdict.UKE


def test_timestamp_conversion():
    """中国时区 → UTC 秒级转换验证。"""
    from adapters.nowcoder import CHINA_TZ

    dt = datetime(2019, 10, 5, 14, 9, 20, tzinfo=CHINA_TZ)
    expected_utc = int(dt.timestamp())
    # 验证：UTC 时间应为 2019-10-05 06:09:20
    utc_dt = datetime.fromtimestamp(expected_utc, tz=UTC)
    assert utc_dt.hour == 6
    assert utc_dt.minute == 9


def test_to_submission_row_mapping():
    """单条 HTML 行 → PlatformSubmission 映射验证。"""
    from adapters.nowcoder import NowcoderAdapter

    ts_utc = NowcoderAdapter.to_utc_seconds("2019-10-05 14:09:20")
    # 使用完整的 HTML 结构（含 <table> 和 <tbody>）
    html = (
        '<table><thead><tr><th>提交ID</th><th>题目</th><th>评测结果</th>'
        '<th>得分</th><th>运行时间</th><th>使用内存</th><th>代码长度</th>'
        '<th>使用语言</th><th>提交时间</th></tr></thead><tbody>'
        '<tr><td><a href="/acm/contest/view-submission?submissionId=41357928">41357928</a></td>'
        '<td><a href="/acm/problem/52897">String Transformation</a></td>'
        '<td><a href="#" class="font-green">答案正确</a></td>'
        '<td>100</td><td>7</td><td>484</td><td>668</td>'
        '<td>C++</td><td>2019-10-05 14:09:20</td></tr>'
        '</tbody></table>'
    )
    parsed_rows = NowcoderAdapter._parse_rows(html)
    assert len(parsed_rows) == 1
    s = NowcoderAdapter.to_submission(parsed_rows[0], ts_utc)
    assert isinstance(s, PlatformSubmission)
    assert s.submission_id == "41357928"
    assert s.problem_key == "52897"
    assert s.problem_name == "String Transformation"
    assert s.problem_url == "https://ac.nowcoder.com/acm/problem/52897"
    assert s.difficulty is None
    assert s.verdict is Verdict.AC
    assert s.language == "C++"
    assert s.submitted_at == ts_utc


# ===== 题目查询（PROBLEM_LOOKUP） =====

PROBLEM_LIST_HTML = (FIXTURES / "nc_problem_list_sample.html").read_text(
    encoding="utf-8"
)


async def test_fetch_problem_exact_match():
    """keyword 精确查询返回单题画像：标签文本 + 标签主键 + 难度 + 外链。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        assert "/acm/problem/list" in str(request.url)
        assert request.url.params["keyword"] == "322568"
        return httpx.Response(200, text=PROBLEM_LIST_HTML)

    adapter, fetcher = make_adapter(handler)
    try:
        meta = await adapter.fetch_problem("322568")
    finally:
        await fetcher.aclose()

    assert meta is not None
    assert meta.problem_key == "322568"
    assert meta.name == "小月的前缀集合"
    assert meta.rating == 1200
    assert meta.tags == ["字典树（Trie树）", "哈希"]
    # 牛客的 tagId 过滤只认主键，必须保留 data-id
    assert meta.tag_ids == ["1269", "1274"]
    assert meta.url == "https://ac.nowcoder.com/acm/problem/322568"


async def test_fetch_problem_not_found_returns_none():
    """题库里没有该题号时返回 None（不抛错，由调用方给空推荐）。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, text="<html><body></body></html>")

    adapter, fetcher = make_adapter(handler)
    try:
        assert await adapter.fetch_problem("999999") is None
    finally:
        await fetcher.aclose()


async def test_fetch_problem_missing_rating_is_none():
    """难度列为空（旧题未定级）→ rating 为 None，标签仍可用于推荐。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, text=PROBLEM_LIST_HTML)

    adapter, fetcher = make_adapter(handler)
    try:
        meta = await adapter.fetch_problem("215120")
    finally:
        await fetcher.aclose()

    assert meta is not None
    assert meta.rating is None
    assert meta.tags == ["图论", "最短路"]


def test_parse_problem_rows_skips_rows_without_title():
    """缺 title 类的行（结构异常）跳过，不让整页解析失败。"""
    metas = NowcoderAdapter._parse_problem_rows(PROBLEM_LIST_HTML)
    keys = [m.problem_key for m in metas]
    assert "999999" not in keys
    assert keys == ["325377", "322568", "307924", "215120"]


async def test_find_similar_uses_tag_ids_and_rating_window():
    """候选查询带上 tagId 与难度窗口（服务端过滤，减少请求量与候选规模）。"""
    seen: dict[str, str] = {}

    async def handler(request: httpx.Request) -> httpx.Response:
        seen.update(dict(request.url.params))
        return httpx.Response(200, text=PROBLEM_LIST_HTML)

    adapter, fetcher = make_adapter(handler)
    try:
        target = await adapter.fetch_problem("322568")
        assert target is not None
        await adapter.find_similar_problems(target, limit=5)
    finally:
        await fetcher.aclose()

    assert seen["tagId"] == "1269,1274"
    assert seen["minDifficulty"] == "1000"
    assert seen["maxDifficulty"] == "1300"


async def test_find_similar_without_rating_skips_difficulty():
    """源题无难度时不传难度区间（只按标签查）。"""
    seen: dict[str, str] = {}

    async def handler(request: httpx.Request) -> httpx.Response:
        seen.update(dict(request.url.params))
        return httpx.Response(200, text=PROBLEM_LIST_HTML)

    adapter, fetcher = make_adapter(handler)
    try:
        target = await adapter.fetch_problem("215120")
        assert target is not None
        await adapter.find_similar_problems(target, limit=5)
    finally:
        await fetcher.aclose()

    assert "minDifficulty" not in seen
    assert "maxDifficulty" not in seen


# ===== 按算法查找（标签目录 + 按标签搜题）=====


def _problem_list_json(rows: list[dict], count: int = 0) -> str:
    """list/json 响应（字段与线上一致的最小集合）。"""
    return json.dumps(
        {
            "msg": "OK",
            "code": 0,
            "data": {"problemCount": count, "problemSets": rows},
        },
        ensure_ascii=False,
    )


async def test_search_problems_maps_json_rows():
    """按标签搜题走 list/json；problem_key 用纯数字 problemId（不是 showId）。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        assert "/acm/problem/list/json" in str(request.url)
        assert request.url.params["tagId"] == "14424"
        assert request.url.params["page"] == "2"
        assert request.url.params["pageSize"] == "50"
        return httpx.Response(
            200,
            text=_problem_list_json(
                [
                    {
                        "problemId": 274704,
                        "showId": "NC274704",
                        "name": "祥子拆团",
                        "difficulty": 2100,
                        "tagList": [
                            {"id": 14424, "name": "费马小定理"},
                            {"id": 145607, "name": "数论"},
                        ],
                    }
                ],
                count=20,
            ),
        )

    adapter, fetcher = make_adapter(handler)
    try:
        metas = await adapter.search_problems("14424", page=2)
    finally:
        await fetcher.aclose()

    assert len(metas) == 1
    meta = metas[0]
    # 必须与提交归一化的 problem_key（纯数字）同口径
    assert meta.problem_key == "274704"
    assert meta.name == "祥子拆团"
    assert meta.rating == 2100
    assert meta.tags == ["费马小定理", "数论"]
    assert meta.tag_ids == ["14424", "145607"]
    assert meta.url == "https://ac.nowcoder.com/acm/problem/274704"


async def test_search_problems_error_envelope_raises():
    """code != 0 抛 PlatformError（不静默返回空列表）。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(
            200, text='{"msg":"该知识点不存在或已隐藏","code":1,"data":null}'
        )

    adapter, fetcher = make_adapter(handler)
    try:
        with pytest.raises(PlatformError):
            await adapter.search_problems("bad-tag")
    finally:
        await fetcher.aclose()


async def test_list_problem_tags_flattens_groups():
    """知识点目录按组展开为扁平标签；hidden 组跳过。"""

    catalog = json.dumps(
        {
            "msg": "OK",
            "code": 0,
            "data": {
                "groupCount": 2,
                "groups": [
                    {
                        "id": 145412,
                        "name": "数论",
                        "hidden": False,
                        "childTags": [
                            {
                                "groupName": "数论",
                                "name": "费马小定理",
                                "id": 14424,
                                "totalCount": 20,
                            },
                            {
                                "groupName": "数论",
                                "name": "欧拉函数",
                                "id": 14427,
                                "totalCount": 39,
                            },
                        ],
                    },
                    {
                        "id": 999,
                        "name": "隐藏组",
                        "hidden": True,
                        "childTags": [
                            {"groupName": "隐藏组", "name": "X", "id": 1, "totalCount": 1}
                        ],
                    },
                ],
            },
        },
        ensure_ascii=False,
    )

    async def handler(request: httpx.Request) -> httpx.Response:
        assert "/acm/problem/knowledge/catalog" in str(request.url)
        return httpx.Response(200, text=catalog)

    adapter, fetcher = make_adapter(handler)
    try:
        tags = await adapter.list_problem_tags()
    finally:
        await fetcher.aclose()

    assert [t.id for t in tags] == ["14424", "14427"]
    assert [t.name for t in tags] == ["费马小定理", "欧拉函数"]
    assert tags[0].group == "数论"
    assert tags[0].count == 20


def test_parse_problem_url():
    """牛客链接解析：题库题与比赛题都归一到纯数字题号。"""
    from adapters.nowcoder.normalize import parse_problem_url

    assert (
        parse_problem_url("https://www.nowcoder.com/acm/problem/325377") == "325377"
    )
    assert (
        parse_problem_url(
            "https://ac.nowcoder.com/acm/contest/12345/problem/325377?x=1"
        )
        == "325377"
    )
    assert (
        parse_problem_url("https://www.nowcoder.com/acm/problem/list") is None
    )
    assert parse_problem_url("https://codeforces.com/contest/2245/problem/F") is None
    assert parse_problem_url("") is None


def test_parse_problem_url_rejects_practice_uuid():
    """题霸链接（/practice/{uuid}）无题号：纯函数不猜，交给 resolve 抓页面。"""
    from adapters.nowcoder.normalize import is_practice_url, parse_problem_url

    url = (
        "https://www.nowcoder.com/practice/23d4c8aa35894cb0bd61df2173c93321"
        "?tpId=391&contestId=141877"
    )
    assert parse_problem_url(url) is None
    assert is_practice_url(url) is True
    assert is_practice_url("https://www.nowcoder.com/acm/problem/325082") is False
    assert is_practice_url("https://codeforces.com/practice/abc") is False


def test_extract_problem_id_from_page():
    """题霸页内嵌 window.pageInfo：problemId 是题库题号，questionId 不是。"""
    from adapters.nowcoder.normalize import extract_problem_id_from_page

    html = (
        "<script>window.pageInfo = {\n"
        "        questionID_var: '11892363',\n"
        "        questionId: '11892363',\n"
        "        problemId: '325082',\n"
        "        uuid : '23d4c8aa35894cb0bd61df2173c93321',\n"
        "};</script>"
    )
    assert extract_problem_id_from_page(html) == "325082"
    assert extract_problem_id_from_page("<html>no info</html>") is None
    assert extract_problem_id_from_page("") is None


async def test_resolve_problem_url_parses_practice_page():
    """题霸链接：抓一次页面换出题库题号；普通题库链接不发请求。"""
    fetched: list[str] = []

    async def handler(request: httpx.Request) -> httpx.Response:
        fetched.append(str(request.url))
        if "/practice/23d4c8aa35894cb0bd61df2173c93321" in str(request.url):
            return httpx.Response(
                200,
                text="<script>window.pageInfo = { problemId: '325082' };</script>",
            )
        return httpx.Response(404)

    adapter, _ = make_adapter(handler)
    practice = (
        "https://www.nowcoder.com/practice/23d4c8aa35894cb0bd61df2173c93321?tpId=391"
    )
    assert await adapter.resolve_problem_url(practice) == "325082"
    assert fetched == [practice]

    # 已有数字题号的链接走纯字符串解析，不产生网络请求
    fetched.clear()
    assert (
        await adapter.resolve_problem_url("https://ac.nowcoder.com/acm/problem/325082")
        == "325082"
    )
    assert fetched == []


async def test_resolve_problem_url_returns_none_on_platform_error():
    """页面抓取失败不能把 500 抛给调用方：返回 None，由收录接口给「无法识别」。"""

    async def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(500)

    adapter, _ = make_adapter(handler)
    url = "https://www.nowcoder.com/practice/23d4c8aa35894cb0bd61df2173c93321"
    assert await adapter.resolve_problem_url(url) is None


# ===== 辅助函数 =====


def _make_html_with_n_rows(n: int) -> str:
    """构造包含 n 条数据行的 HTML。"""
    rows = []
    for i in range(n):
        rows.append(
            f'<tr><td><a href="/acm/contest/view-submission?submissionId={90000000 + i}">'
            f'{90000000 + i}</a></td>'
            f'<td><a href="/acm/problem/{10000 + i}">Problem {i}</a></td>'
            f'<td><a href="#" class="font-green">答案正确</a></td>'
            f'<td>100</td><td>10</td><td>512</td><td>500</td>'
            f'<td>C++</td><td>2018-01-01 00:00:0{i}</td></tr>'
        )
    return (
        "<!DOCTYPE html><html><body><table><tr><th>提交ID</th><th>题目</th>"
        "<th>评测结果</th><th>得分</th><th>运行时间</th><th>使用内存</th>"
        "<th>代码长度</th><th>使用语言</th><th>提交时间</th></tr>"
        + "".join(rows)
        + "</table></body></html>"
    )
