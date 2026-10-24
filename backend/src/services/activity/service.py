"""activity 业务编排门面：用户组管理、账号 CRUD / 绑定验证、触发同步、聚合读取。

依赖方向严格单向 routers → services → modules → adapters；
adapter 只允许被本服务与 modules/activity/sync.py 触碰。

用户组 = data/user/<user_id>/ 目录（目录名即组名，支持中文）；
服务层维护"当前用户组"（单机本地应用，内存态，默认 default），
其余 API 一律作用于当前组。信息卡（ID/签名/头像）存于组内 profile.json，
与组名分离、独立编辑。
"""

import asyncio
import logging
import time
from collections import Counter
from datetime import datetime, tzinfo

from adapters import REGISTRY, HttpFetcher
from adapters.base import (
    AuthExpiredError,
    AuthMode,
    BrowserLoginCancelledError,
    Capability,
    Credentials,
    PlatformAdapter,
    PlatformError,
    ProblemMeta,
    TagMeta,
    UserNotFoundError,
    Verdict,
)
from core.config import Settings, get_settings
from core.exceptions import (
    BadGatewayError,
    BadRequestError,
    ConflictError,
    NotFoundError,
)
from modules.activity import aggregate, problemset, review
from modules.activity import store as activity_store
from modules.activity.models import (
    DEFAULT_USER_ID,
    Account,
    ManualReviewItem,
    Submission,
    TodoItem,
)
from modules.activity.refine import RefineEngine
from modules.activity.schemas import (
    BindIn,
    BoundAccountOut,
    BrowserLoginStatusOut,
    DayActivityOut,
    GroupCreateIn,
    GroupOut,
    GroupRenameIn,
    GroupsOut,
    ManualReviewIn,
    ManualReviewOut,
    OverviewOut,
    OverviewTotalsOut,
    PlatformMetaOut,
    PlatformsOut,
    ProblemTagOut,
    ProblemTagsOut,
    ProfileOut,
    ProfileUpdateIn,
    RecommendEmptyReason,
    RecommendItemOut,
    RecommendOut,
    RefineStatusOut,
    ReviewItemOut,
    ReviewOut,
    ReviewStatusOut,
    SearchProblemOut,
    SearchProblemsOut,
    SubmissionOut,
    SubmissionsOut,
    TodoAddOut,
    TodoIn,
    TodoItemOut,
    TodoOut,
    TodoTagOut,
    VerifyIn,
    VerifyOut,
)
from modules.activity.sync import SyncEngine

logger = logging.getLogger("xcpc.service.activity")

# 近期提交条数上限：全部历史按时间倒序取最后 N 条，
# 不按时间窗口过滤（保证"近期没做题"的账号也能看到最近记录）
RECENT_LIMIT = 200
# 信息卡头像 data URL 长度上限（512px JPEG 约 40-80KB，base64 后留足余量）
AVATAR_MAX_CHARS = 500_000
# browser-login 抓取到的凭据暂存 TTL（bind 消费，凭据不经前端）
PENDING_CREDENTIALS_TTL = 600.0
# browser-login 用户登录操作超时（秒）
BROWSER_LOGIN_TIMEOUT = 180.0
# 按算法搜题每页条数（与平台题库页 pageSize 对齐，末页按「本页是否满」判定）
REVIEW_SEARCH_PAGE_SIZE = 50


class ActivityService:
    """训练统计服务。读写均作用于 data/user/<userid>/ 文件事实来源。"""

    def __init__(
        self, settings: Settings, fetcher: HttpFetcher | None = None
    ) -> None:
        self._settings = settings
        # 测试可注入 MockTransport 的 fetcher，避免真实外呼
        self._fetcher = fetcher or HttpFetcher()
        self._adapters: dict[str, PlatformAdapter] = {
            pid: cls(self._fetcher) for pid, cls in REGISTRY.items()
        }
        self._engine = SyncEngine(
            settings.user_data_dir,
            self._adapters,
            full_window_days=settings.activity_window_days,
            full_min_rows=settings.activity_full_min_rows,
        )
        # 精细化同步引擎（UNAC → 细分结果；与同步共享账号锁，见 §6.5）
        self._refine = RefineEngine(settings.user_data_dir, self._adapters, self._engine)
        # 用户组初始化：仅在"一个组都不存在"时（真正的首次运行）创建 default；
        # 已有组时（default 被删除/重命名）不再重建，当前组回落到 default
        # 或现存首个组（目录即组，见 store.list_groups）
        groups = activity_store.list_groups(settings.user_data_dir)
        if not groups:
            activity_store.create_group(settings.user_data_dir, DEFAULT_USER_ID)
            groups = [DEFAULT_USER_ID]
        # 当前用户组（内存态；单机本地应用，前端切组时切换）
        self._current_group = (
            DEFAULT_USER_ID if DEFAULT_USER_ID in groups else groups[0]
        )
        # browser-login 会话状态（按平台互斥）与待消费凭据暂存（内存态，凭据不经前端）
        self._browser_logins: dict[str, BrowserLoginStatusOut] = {}
        self._pending_credentials: dict[tuple[str, str], tuple[Credentials, float]] = {}

    async def aclose(self) -> None:
        await self._fetcher.aclose()

    def _adapter(self, platform: str) -> PlatformAdapter:
        adapter = self._adapters.get(platform)
        if adapter is None:
            raise BadRequestError(f"不支持的平台: {platform}")
        return adapter

    def _store(self) -> activity_store.UserStore:
        """当前用户组的 store。"""
        return activity_store.UserStore(
            self._settings.user_data_dir, self._current_group
        )

    def _require_group(self, name: str) -> None:
        """组存在性校验（目录即组）。"""
        if name not in activity_store.list_groups(self._settings.user_data_dir):
            raise NotFoundError(f"用户组不存在: {name}")

    # ===== 用户组管理 =====

    def groups(self) -> GroupsOut:
        root = self._settings.user_data_dir
        return GroupsOut(
            groups=[
                GroupOut(name=g, current=(g == self._current_group))
                for g in activity_store.list_groups(root)
            ]
        )

    def create_group(self, payload: GroupCreateIn) -> GroupOut:
        """新建用户组并切换过去（信息卡 ID 初始为组名，之后独立编辑）。"""
        name = activity_store.create_group(
            self._settings.user_data_dir, payload.name.strip()
        )
        self._current_group = name
        return GroupOut(name=name, current=True)

    def rename_group(self, name: str, payload: GroupRenameIn) -> GroupsOut:
        """用户组目录改名（数据归属不变）；当前组被改名时同步切换。"""
        new_name = activity_store.rename_group(
            self._settings.user_data_dir, name, payload.newName.strip()
        )
        if self._current_group == name:
            self._current_group = new_name
        return self.groups()

    def delete_group(self, name: str) -> None:
        """物理删除用户组（含全部数据）；当前组被删则回退到剩余组。"""
        remaining = activity_store.list_groups(self._settings.user_data_dir)
        if len(remaining) <= 1 and name in remaining:
            raise BadRequestError("至少保留一个用户组")
        activity_store.delete_group(self._settings.user_data_dir, name)
        self._engine.drop_user(name)
        self._refine.drop_user(name)
        if self._current_group == name:
            rest = activity_store.list_groups(self._settings.user_data_dir)
            self._current_group = rest[0] if rest else DEFAULT_USER_ID

    def switch_group(self, name: str) -> GroupOut:
        """切换当前用户组。"""
        self._require_group(name)
        self._current_group = name
        return GroupOut(name=name, current=True)

    # ===== 信息卡（profile）=====

    def current_profile(self) -> ProfileOut:
        profile = self._store().load_profile()
        return ProfileOut(
            id=profile.id, signature=profile.signature, avatar=profile.avatar
        )

    def update_profile(self, payload: ProfileUpdateIn) -> ProfileOut:
        profile = self._store().load_profile()
        if payload.id is not None:
            profile.id = payload.id.strip()
        if payload.signature is not None:
            profile.signature = payload.signature.strip()
        # avatar 用 fields_set 判断：显式传 null 表示清除头像
        if "avatar" in payload.model_fields_set:
            avatar = payload.avatar
            if avatar is not None and len(avatar) > AVATAR_MAX_CHARS:
                raise BadRequestError("头像文件过大，请换一张小一点的图片")
            profile.avatar = avatar or None
        self._store().save_profile(profile)
        return ProfileOut(
            id=profile.id, signature=profile.signature, avatar=profile.avatar
        )

    # ===== 平台元数据 =====

    def platforms(self) -> PlatformsOut:
        profile = self._store().load_profile()
        metas: list[PlatformMetaOut] = []
        for pid, adapter in self._adapters.items():
            account = next(
                (acc for acc in profile.accounts if acc.platform == pid), None
            )
            metas.append(
                PlatformMetaOut(
                    id=pid,
                    name=adapter.name,
                    capabilities=sorted(adapter.capabilities, key=lambda c: c.value),
                    auth=adapter.auth.value,
                    browserLogin=self._browser_login_available(adapter),
                    homepageUrl=adapter.homepage_url,
                    account=self._account_out(account) if account else None,
                )
            )
        return PlatformsOut(platforms=metas)

    @staticmethod
    def _browser_login_available(adapter: PlatformAdapter) -> bool:
        """一键登录可用 = cookie 平台 + adapter 实现可选契约 + Playwright 已安装。"""
        if adapter.auth != AuthMode.COOKIE:
            return False
        probe = getattr(adapter, "browser_login_available", None)
        return bool(probe and probe())

    def _account_out(self, account: Account) -> BoundAccountOut:
        status = self._engine.status_of(
            self._current_group, account.platform, account.handle
        )
        last_sync_at = status.last_synced_at
        if last_sync_at is None and account.last_sync_ok_at:
            # 内存状态缺失（重启后/同步中）：回退到档案记录的最近同步成功时刻。
            # 不可用游标 last_synced_at——那是数据水位（数据新到哪），
            # 会显示成数据水龄（如"71 天前同步"的假象）。
            last_sync_at = datetime.fromtimestamp(account.last_sync_ok_at).astimezone()
        return BoundAccountOut(
            platform=account.platform,
            handle=account.handle,
            displayName=account.display_name,
            lastSyncAt=last_sync_at,
            syncState=status.state.value,
            syncError=status.error,
            syncErrorCode=status.error_code,
            syncProgress=status.progress,
        )

    # ===== 绑定验证 =====

    async def verify(self, payload: VerifyIn) -> VerifyOut:
        adapter = self._adapter(payload.platform)
        if Capability.USER_INFO not in adapter.capabilities:
            raise BadRequestError(f"平台 {payload.platform} 不支持绑定验证")
        handle = payload.handle.strip()
        if not handle:
            raise BadRequestError("请输入账号标识")
        credentials = (
            Credentials.model_validate(payload.credentials)
            if payload.credentials
            else None
        )
        # cookie 授权平台：验证即需凭据（存在性校验 + 凭据有效性试拉）
        if adapter.auth == AuthMode.COOKIE and credentials is None:
            raise BadRequestError(f"平台 {payload.platform} 需要登录凭据（cookie）")
        try:
            info = await adapter.verify(handle, credentials)
        except UserNotFoundError as exc:
            raise BadRequestError(str(exc)) from exc
        except AuthExpiredError as exc:
            # 凭据在绑定当下即无效：转 400 引导重新录入，不放行死凭据
            raise BadRequestError(str(exc)) from exc
        except PlatformError as exc:
            raise BadGatewayError(f"平台暂时不可用：{exc}") from exc
        return VerifyOut(
            platform=payload.platform,
            handle=info.handle,
            displayName=info.display_name,
            avatar=info.avatar,
        )

    # ===== 浏览器一键登录（cookie 平台） =====

    async def start_browser_login(self, platform: str) -> None:
        """启动浏览器登录会话（202 立即返回，前端轮询 status）。

        会话按平台互斥；凭据抓取成功后由 adapter 完成验证并暂存
        （PENDING_CREDENTIALS_TTL），bind 时消费——凭据不经前端。
        """
        adapter = self._adapter(platform)
        if adapter.auth != AuthMode.COOKIE:
            raise BadRequestError(f"平台 {platform} 无需浏览器登录")
        runner = getattr(adapter, "run_browser_login", None)
        if runner is None:
            raise BadRequestError(f"平台 {platform} 不支持浏览器登录")
        if not self._browser_login_available(adapter):
            raise BadRequestError(
                "一键登录不可用（未安装 browser-login 依赖），请改用手动粘贴 cookie"
            )
        current = self._browser_logins.get(platform)
        if current is not None and current.state == "waiting":
            raise ConflictError("已有进行中的登录会话，请先完成或关闭登录窗口")
        self._browser_logins[platform] = BrowserLoginStatusOut(state="waiting")
        asyncio.create_task(self._run_browser_login(platform, adapter))

    def browser_login_status(self, platform: str) -> BrowserLoginStatusOut:
        """查询登录会话状态（无会话时返回 error 引导重新发起）。"""
        self._adapter(platform)
        return self._browser_logins.get(
            platform,
            BrowserLoginStatusOut(state="error", error="无进行中的登录会话"),
        )

    async def _run_browser_login(self, platform: str, adapter: PlatformAdapter) -> None:
        try:
            credentials, info = await adapter.run_browser_login(  # type: ignore[attr-defined]
                timeout=BROWSER_LOGIN_TIMEOUT
            )
            key = (platform, info.handle)
            self._pending_credentials[key] = (
                credentials,
                time.monotonic() + PENDING_CREDENTIALS_TTL,
            )
            self._browser_logins[platform] = BrowserLoginStatusOut(
                state="success",
                handle=info.handle,
                displayName=info.display_name,
                avatar=info.avatar,
            )
        except BrowserLoginCancelledError:
            self._browser_logins[platform] = BrowserLoginStatusOut(state="canceled")
        except TimeoutError:
            self._browser_logins[platform] = BrowserLoginStatusOut(state="timeout")
        except PlatformError as exc:
            logger.warning("浏览器登录失败 [%s] %s", platform, exc)
            self._browser_logins[platform] = BrowserLoginStatusOut(
                state="error", error=str(exc)
            )
        except Exception as exc:  # 兜底降级，不让后台任务悬空
            logger.exception("浏览器登录意外异常 [%s]", platform)
            self._browser_logins[platform] = BrowserLoginStatusOut(
                state="error", error=str(exc)
            )

    def _take_pending_credentials(
        self, platform: str, handle: str
    ) -> Credentials | None:
        """消费 browser-login 暂存凭据（一次性；过期即弃）。"""
        key = (platform, handle)
        entry = self._pending_credentials.pop(key, None)
        if entry is None:
            return None
        credentials, expires_at = entry
        if time.monotonic() > expires_at:
            return None
        return credentials

    # ===== 绑定 / 解绑 =====

    async def bind(self, payload: BindIn) -> BoundAccountOut:
        adapter = self._adapter(payload.platform)
        handle = payload.handle.strip()
        if not handle:
            raise BadRequestError("请输入账号标识")
        credentials = (
            Credentials.model_validate(payload.credentials)
            if payload.credentials
            else None
        )
        # cookie 授权平台：显式凭据优先，否则消费 browser-login 暂存凭据
        if adapter.auth == AuthMode.COOKIE and credentials is None:
            credentials = self._take_pending_credentials(payload.platform, handle)
        if adapter.auth == AuthMode.COOKIE and credentials is None:
            raise BadRequestError(
                f"平台 {payload.platform} 需要登录凭据（一键登录或粘贴 cookie）"
            )
        store = self._store()
        # 换绑：每个平台每用户组只保留一个账号，旧账号连同本地数据与凭据删除
        profile = store.load_profile()
        for acc in profile.accounts:
            if acc.platform == payload.platform:
                store.remove_account(acc.platform, acc.handle)
                self._engine.drop_status(
                    self._current_group, acc.platform, acc.handle
                )
                self._refine.drop_account(self._current_group, acc.platform, acc.handle)
                break
        account = Account(
            platform=payload.platform,
            handle=handle,
            display_name=(payload.displayName or "").strip() or None,
        )
        store.save_account(account)
        if credentials is not None:
            store.save_account_secrets(payload.platform, handle, credentials)
        # 首次同步后台异步执行，前端轮询 /sync/status
        asyncio.create_task(self._safe_sync(payload.platform, handle))
        return self._account_out(account)

    def unbind(self, platform: str, handle: str) -> None:
        self._adapter(platform)
        handle = handle.strip()
        store = self._store()
        profile = store.load_profile()
        if not any(
            acc.platform == platform and acc.handle == handle
            for acc in profile.accounts
        ):
            raise NotFoundError(f"账号未绑定: {platform}/{handle}")
        store.remove_account(platform, handle)
        self._engine.drop_status(self._current_group, platform, handle)
        self._refine.drop_account(self._current_group, platform, handle)

    async def update_credentials(
        self,
        platform: str,
        handle: str,
        credentials: Credentials | None = None,
    ) -> BoundAccountOut:
        """更新已绑定账号的凭据（仅 cookie 平台）：不删数据、不重置游标。

        验证新凭据有效性后仅覆盖 secrets.json，保留 submissions 与同步状态。
        credentials 为 None 时尝试消费 browser-login 暂存凭据。
        """
        adapter = self._adapter(platform)
        handle = handle.strip()
        store = self._store()
        profile = store.load_profile()
        account = next(
            (
                acc
                for acc in profile.accounts
                if acc.platform == platform and acc.handle == handle
            ),
            None,
        )
        if account is None:
            raise NotFoundError(f"账号未绑定: {platform}/{handle}")
        if adapter.auth != AuthMode.COOKIE:
            raise BadRequestError(f"平台 {platform} 无需凭据更新")

        # credentials 为 None 时尝试消费 browser-login 暂存凭据
        if credentials is None:
            credentials = self._take_pending_credentials(platform, handle)
        if credentials is None:
            raise BadRequestError(
                f"平台 {platform} 需要登录凭据（一键登录或粘贴 cookie）"
            )

        # 验证新凭据有效性（存在性 + 凭据试拉）
        try:
            await adapter.verify(handle, credentials)
        except AuthExpiredError as exc:
            raise BadRequestError(str(exc)) from exc
        except PlatformError as exc:
            raise BadGatewayError(f"平台暂时不可用：{exc}") from exc
        # 仅覆盖凭据，不碰账号元数据、不删提交数据、不重置游标
        store.save_account_secrets(platform, handle, credentials)
        # 清除凭据过期的错误状态，让用户可以立即重新同步
        self._engine.drop_status(self._current_group, platform, handle)
        # 凭据更新成功后自动触发一次同步
        asyncio.create_task(self._safe_sync(platform, handle))
        return self._account_out(account)

    # ===== 聚合读取 =====

    def _submissions(self, platform: str | None) -> list[Submission]:
        """当前用户组全部账号（可按平台过滤）的提交合并。"""
        store = self._store()
        profile = store.load_profile()
        out: list[Submission] = []
        for acc in profile.accounts:
            if platform is not None and acc.platform != platform:
                continue
            items, skipped = store.load_submissions(acc.platform, acc.handle)
            if skipped:
                logger.warning(
                    "提交数据 %d 行损坏被跳过 [%s/%s]",
                    skipped,
                    acc.platform,
                    acc.handle,
                )
            out.extend(items)
        return out

    def overview(self, platform: str | None) -> OverviewOut:
        if platform is not None:
            self._adapter(platform)
        submissions = self._submissions(platform)
        tz = datetime.now().astimezone().tzinfo
        return OverviewOut(
            totals=OverviewTotalsOut(**aggregate.overview_stats(submissions, tz=tz)),
            daily=[
                DayActivityOut(**d)
                for d in aggregate.daily_series(
                    submissions, tz=tz, days=self._settings.activity_window_days
                )
            ],
        )

    # ===== 错题本与同类推荐（review，设计见 activity/review.md） =====

    def review_list(self, platform: str | None, status: str) -> ReviewOut:
        """错题本：未通过 + 挣扎题 + 手动收录（按最近时间倒序）。

        status 取 all / unsolved / struggled / collected；
        platform 为 None 表示全部平台。纯本地读取，不触发同步。
        """
        if platform is not None:
            self._adapter(platform)
        if status not in ("all", "unsolved", "struggled", "collected"):
            raise BadRequestError(f"不支持的状态筛选: {status}")

        store = self._store()
        profile = store.load_profile()
        all_submissions = self._submissions(platform)
        records = review.review_records(all_submissions)
        solved = review.solved_keys(all_submissions)
        manual = store.load_manual_review()
        if platform is not None:
            manual = [m for m in manual if m.platform == platform]
        merged = review.merge_review(records, manual, solved)
        if status != "all":
            merged = [r for r in merged if r.status == status]
        # 待办键集（本地文件读取，零网络）：前端据 inTodo 把按钮置为「已待办」
        todo_keys = {(it.platform, it.problem_key) for it in store.load_todo()}

        tz = datetime.now().astimezone().tzinfo
        capable = {
            pid
            for pid, adapter in self._adapters.items()
            if Capability.PROBLEMSET in adapter.capabilities
            or Capability.PROBLEM_LOOKUP in adapter.capabilities
        }
        return ReviewOut(
            items=[
                ReviewItemOut(
                    platform=r.platform,
                    problemKey=r.problem_key,
                    problemName=r.problem_name,
                    problemUrl=r.problem_url,
                    difficulty=r.difficulty,
                    status=r.status,
                    failCount=r.fail_count,
                    lastAttemptAt=datetime.fromtimestamp(r.last_attempt_at, tz=tz),
                    canRecommend=r.platform in capable,
                    manual=r.manual,
                    inTodo=(r.platform, r.problem_key) in todo_keys,
                )
                for r in merged
            ],
            bound=bool(profile.accounts),
            synced=any(acc.last_synced_at is not None for acc in profile.accounts),
        )

    # ===== 手动收录（链接 / 题号） =====

    async def add_manual_review(self, payload: ManualReviewIn) -> ManualReviewOut:
        """收录一题：按链接解析，或按 (platform, problemKey) 直接收录。

        - 链接解析用各 adapter 的纯函数 parse_problem_url（顺注册序，先命中者赢）；
        - 解析成功后再查一次平台画像补题名/难度；查不到也允许收录（比赛专属题、
          未公开题、平台暂时不可达都会走到这里），题名退化为链接兜底；
        - 同一 (platform, problem_key) 幂等，created=False 表示已存在。
        """
        has_url = bool(payload.url and payload.url.strip())
        has_key = bool(payload.platform and payload.problemKey)
        if has_url == has_key:
            raise BadRequestError("请提供题目链接，或同时提供平台与题号")

        if has_url:
            platform, key = await self._parse_manual_url(payload.url or "")
        else:
            platform = payload.platform or ""
            self._adapter(platform)  # 平台不存在 → 400
            key = (payload.problemKey or "").strip()
            if not key:
                raise BadRequestError("题号不能为空")

        adapter = self._adapter(platform)
        meta = await self._fetch_problem_quietly(adapter, key)
        store = self._store()
        item = ManualReviewItem(
            platform=platform,
            problem_key=key,
            problem_name=(meta.name if meta else "") or key,
            problem_url=(meta.url if meta else "")
            or self._fallback_problem_url(platform, key),
            difficulty=meta.rating if meta else None,
            added_at=int(time.time()),
        )
        saved, created = store.add_manual_review(item)
        return ManualReviewOut(
            item=self._manual_to_out(saved), created=created
        )

    async def _parse_manual_url(self, url: str) -> tuple[str, str]:
        """链接 → (平台, 题号)。

        两轮：先跑所有平台的纯字符串解析（零请求，覆盖绝大多数链接），
        都没命中再让各平台尝试 resolve_problem_url（目前只有牛客题霸链接
        需要抓页面换题号）。两轮分开是为了不让常规链接白跑网络请求。
        """
        adapters = [
            (platform, adapter)
            for platform, adapter in self._adapters.items()
            if Capability.PROBLEM_LOOKUP in adapter.capabilities
            or Capability.PROBLEMSET in adapter.capabilities
        ]
        for platform, adapter in adapters:
            key = adapter.parse_problem_url(url)
            if key:
                return platform, key
        for platform, adapter in adapters:
            key = await adapter.resolve_problem_url(url)
            if key:
                return platform, key
        raise BadRequestError(
            "无法识别该链接，支持 Codeforces / 牛客竞赛 / 洛谷的题目页"
        )

    async def _fetch_problem_quietly(
        self, adapter: PlatformAdapter, problem_key: str
    ) -> ProblemMeta | None:
        """取题目画像；任何平台故障都降级为 None（收录不依赖画像成功）。"""
        try:
            if Capability.PROBLEM_LOOKUP in adapter.capabilities:
                return await adapter.fetch_problem(problem_key)
            if Capability.PROBLEMSET in adapter.capabilities:
                cached = await self._problemset(adapter.platform_id)
                if cached is None:
                    return None
                return next(
                    (p for p in cached.items if p.problem_key == problem_key), None
                )
        except (PlatformError, AuthExpiredError) as exc:
            logger.warning(
                "题目画像获取失败 [%s/%s]：%s", adapter.platform_id, problem_key, exc
            )
        return None

    @staticmethod
    def _fallback_problem_url(platform: str, problem_key: str) -> str:
        """画像查不到时的外链兜底（按平台 URL 规则拼题号）。"""
        if platform == "codeforces":
            return f"https://codeforces.com/problemset/problem/{problem_key}"
        if platform == "nowcoder":
            return f"https://ac.nowcoder.com/acm/problem/{problem_key}"
        if platform == "luogu":
            return f"https://www.luogu.com.cn/problem/{problem_key}"
        return ""

    def _manual_to_out(self, item: ManualReviewItem) -> ReviewItemOut:
        """收纳条目 → API 输出（收录恒为 collected / failCount=0）。"""
        tz = datetime.now().astimezone().tzinfo
        adapter = self._adapters.get(item.platform)
        can_recommend = adapter is not None and (
            Capability.PROBLEMSET in adapter.capabilities
            or Capability.PROBLEM_LOOKUP in adapter.capabilities
        )
        in_todo = any(
            it.platform == item.platform and it.problem_key == item.problem_key
            for it in self._store().load_todo()
        )
        return ReviewItemOut(
            platform=item.platform,
            problemKey=item.problem_key,
            problemName=item.problem_name,
            problemUrl=item.problem_url,
            difficulty=item.difficulty,
            status="collected",
            failCount=0,
            lastAttemptAt=datetime.fromtimestamp(item.added_at, tz=tz),
            canRecommend=can_recommend,
            manual=True,
            inTodo=in_todo,
        )

    def remove_manual_review(self, platform: str, problem_key: str) -> None:
        """删除手动收录项；不存在转 404（不静默成功，便于前端发现状态漂移）。"""
        self._adapter(platform)
        if not self._store().remove_manual_review(platform, problem_key):
            raise NotFoundError(f"收录条目不存在: {platform}/{problem_key}")

    # ===== 待办题目（todo，设计见 activity/todo.md） =====

    async def add_todo(self, payload: TodoIn) -> TodoAddOut:
        """把一道题加进待办：已 AC 拒绝，画像查不到也允许加入。

        - 已 AC → 400（前端不给入口，接口仍要挡住直接调用）；
        - 标签是**加入时的快照**：CF 取题单缓存的 tags，牛客取题库 keyword，
          洛谷把题目页的 tag_ids 映射成名称；任一环节失败 → tags=[]，
          题照样能进待办（只是进不了任何花片）；
        - 同一 (platform, problem_key) 幂等，created=False 表示已存在。
        """
        platform = payload.platform.strip()
        adapter = self._adapter(platform)  # 平台不存在 → 400
        key = (payload.problemKey or "").strip()
        if not key:
            raise BadRequestError("题号不能为空")

        submissions = self._submissions(None)
        if (platform, key) in review.solved_keys(submissions):
            raise BadRequestError("该题已 AC，不需要加入待办")

        meta = await self._fetch_problem_quietly(adapter, key)
        item = TodoItem(
            platform=platform,
            problem_key=key,
            problem_name=(meta.name if meta else "") or key,
            problem_url=(meta.url if meta else "")
            or self._fallback_problem_url(platform, key),
            difficulty=meta.rating if meta else None,
            tags=await self._todo_tags(adapter, meta),
            added_at=int(time.time()),
        )
        store = self._store()
        saved, created = store.add_todo(item)
        return TodoAddOut(
            item=self._todo_to_out(saved, self._todo_status_map(submissions)),
            created=created,
        )

    async def _todo_tags(
        self, adapter: PlatformAdapter, meta: ProblemMeta | None
    ) -> list[str]:
        """取出该题的标签文本快照（查不到就是空：待办不依赖标签成功）。"""
        if meta is None:
            return []
        if meta.tags:
            return list(meta.tags)
        if not meta.tag_ids:
            return []
        # 洛谷等平台题目页只给标签主键，文本需查平台的标签字典
        try:
            catalog = await adapter.list_problem_tags()
        except (PlatformError, AuthExpiredError) as exc:
            logger.warning(
                "标签目录获取失败 [%s]：%s（待办仍可加入，只是无标签）",
                adapter.platform_id,
                exc,
            )
            return []
        by_id = {t.id: t.name for t in catalog}
        names = [by_id[tid] for tid in meta.tag_ids if tid in by_id]
        return list(dict.fromkeys(names))  # 去重且保持平台顺序

    def todo_list(self, platform: str | None, tag: str | None) -> TodoOut:
        """待办列表：已 AC 隐藏 + 平台过滤 + 标签精确匹配 + 花片统计。

        纯本地读取（不触发同步、不查平台）；花片只统计**已 AC 过滤 + 平台过滤**
        之后的集合，不受 tag 参数影响——否则点了一个标签后其余花片会消失，
        无法再切换。
        """
        if platform is not None:
            self._adapter(platform)
        submissions = self._submissions(None)
        solved = review.solved_keys(submissions)
        status_map = self._todo_status_map(submissions)

        store = self._store()
        visible = [
            item
            for item in store.load_todo()
            if (item.platform, item.problem_key) not in solved
            and (platform is None or item.platform == platform)
        ]
        counter: Counter[str] = Counter()
        for item in visible:
            counter.update(set(item.tags))  # 同题重复标签只算一次
        tags = [
            TodoTagOut(name=name, count=count)
            for name, count in sorted(counter.items(), key=lambda kv: (-kv[1], kv[0]))
        ]

        if tag:
            visible = [item for item in visible if tag in item.tags]
        visible.sort(key=lambda it: (-it.added_at, it.platform, it.problem_key))
        return TodoOut(
            items=[self._todo_to_out(item, status_map) for item in visible],
            tags=tags,
        )

    def remove_todo(self, platform: str, problem_key: str) -> None:
        """移出待办；不存在转 404。只碰 review-todo.json，不影响错题本与收录。"""
        self._adapter(platform)
        if not self._store().remove_todo(platform, problem_key):
            raise NotFoundError(f"待办条目不存在: {platform}/{problem_key}")

    @staticmethod
    def _todo_status_map(
        submissions: list[Submission],
    ) -> dict[tuple[str, str], ReviewStatusOut]:
        """(platform, key) → 来源状态：有提交记录即「未通过」，否则「收录」。

        待办里已 AC 的题都被隐藏了，因此有提交记录必然全是失败提交。
        """
        out: dict[tuple[str, str], ReviewStatusOut] = {}
        for s in submissions:
            out.setdefault((s.platform, s.problem_key), "unsolved")
        return out

    @staticmethod
    def _todo_to_out(
        item: TodoItem, status_map: dict[tuple[str, str], ReviewStatusOut]
    ) -> TodoItemOut:
        tz = datetime.now().astimezone().tzinfo
        return TodoItemOut(
            platform=item.platform,
            problemKey=item.problem_key,
            problemName=item.problem_name,
            problemUrl=item.problem_url,
            difficulty=item.difficulty,
            tags=list(item.tags),
            addedAt=datetime.fromtimestamp(item.added_at, tz=tz),
            status=status_map.get((item.platform, item.problem_key), "collected"),
        )

    async def recommend(self, platform: str, problem_key: str) -> RecommendOut:
        """同类推荐：按共同标签 + 相近难度挑未 AC 过的题。

        两种取数据的方式（按 adapter 能力选择，主干不判平台）：
        - PROBLEMSET：全量题单缓存（CF），源题与候选都来自缓存；
        - PROBLEM_LOOKUP：定点查询（牛客），先查源题画像再查同标签候选。
        与错题本隔离：推荐失败只让本次推荐为空，错题列表照常工作。
        """
        adapter = self._adapter(platform)
        has_set = Capability.PROBLEMSET in adapter.capabilities
        has_lookup = Capability.PROBLEM_LOOKUP in adapter.capabilities
        if not has_set and not has_lookup:
            raise BadRequestError(f"平台 {platform} 不支持题单，无法推荐同类题")

        submissions = self._submissions(platform)
        solved = {s.problem_key for s in submissions if s.verdict == Verdict.AC}
        stale = False
        reason: RecommendEmptyReason | None = None

        if has_lookup:
            picked, stale, reason = await self._recommend_by_lookup(
                adapter, platform, problem_key, solved
            )
        else:
            cached = await self._problemset(platform)
            if cached is None:
                return RecommendOut(items=[], stale=True)
            stale = cached.stale
            picked = self._recommend_from_set(cached, problem_key, solved)
            if not picked:
                reason = self._empty_reason_from_set(cached, problem_key)

        # 有结果时 reason 无意义（前端只在空列表时读）
        if picked:
            reason = None

        return RecommendOut(
            items=[
                RecommendItemOut(
                    problemKey=p.problem_key,
                    name=p.name,
                    rating=p.rating,
                    tags=list(p.tags),
                    url=p.url,
                )
                for p in picked
            ],
            stale=stale,
            reason=reason,
        )

    @staticmethod
    def _empty_reason_from_set(
        cached: problemset.CachedProblemset, problem_key: str
    ) -> RecommendEmptyReason:
        """全量题单路径的空结果归因：源题不在题单里 vs 没有匹配。"""
        found = any(p.problem_key == problem_key for p in cached.items)
        return "no_match" if found else "not_indexed"

    @staticmethod
    def _recommend_from_set(
        cached: problemset.CachedProblemset,
        problem_key: str,
        solved: set[str],
    ) -> list[ProblemMeta]:
        """全量题单路径：源题取自缓存，候选即缓存全集。"""
        target = next(
            (p for p in cached.items if p.problem_key == problem_key), None
        )
        if target is None:
            # 题单未命中源题（如新赛题尚未进入题单）：空列表，不报错
            return []
        return review.recommend_problems(
            target=target, candidates=cached.items, solved=solved
        )

    async def _recommend_by_lookup(
        self,
        adapter: PlatformAdapter,
        platform: str,
        problem_key: str,
        solved: set[str],
    ) -> tuple[list[ProblemMeta], bool, RecommendEmptyReason | None]:
        """定点查询路径：查源题画像 → 查同标签候选 → 统一打分。

        返回 (推荐列表, stale, 空结果原因)；平台故障时返回空列表且 stale=True
        （前端据此提示降级），不抛错。
        """
        try:
            target = await adapter.fetch_problem(problem_key)
            if target is None:
                # 题号未收录（如牛客比赛专属题不进题库）：区分于"没有匹配"
                return [], False, "not_indexed"
            candidates = await adapter.find_similar_problems(
                target, limit=review.RECOMMEND_LIMIT
            )
        except PlatformError as exc:
            logger.warning("题目查询失败 [%s/%s]：%s", platform, problem_key, exc)
            return [], True, None

        picked = review.recommend_problems(
            target=target, candidates=candidates, solved=solved
        )
        return picked, False, ("no_match" if not picked else None)

    async def _problemset(self, platform: str) -> problemset.CachedProblemset | None:
        """取题单缓存：TTL 内直接用；过期则刷新，刷新失败回退旧缓存（stale）。"""
        cached = problemset.load(self._settings.data_dir, platform)
        if cached is not None and problemset.is_fresh(
            cached, self._settings.activity_problemset_ttl_seconds
        ):
            return cached

        adapter = self._adapter(platform)
        try:
            items = await adapter.fetch_problemset()
        except PlatformError as exc:
            # 平台故障：有旧缓存就降级使用（标记 stale），没有则不可用
            logger.warning("题单刷新失败 [%s]：%s", platform, exc)
            if cached is None:
                return None
            return problemset.CachedProblemset(
                items=cached.items, fetched_at=cached.fetched_at, stale=True
            )
        problemset.save(self._settings.data_dir, platform, items)
        return problemset.CachedProblemset(
            items=items,
            fetched_at=int(time.time()),
            stale=False,
        )

    # ===== 按算法查找（标签目录 + 按标签搜题） =====

    async def review_tags(self, platform: str) -> ProblemTagsOut:
        """算法标签目录：CF 从题单缓存汇总，牛客/洛谷查平台端点。

        CF 没有数字主键，用标签文本当 id；按出现次数降序，常用的排前面。
        """
        adapter = self._adapter(platform)
        if Capability.PROBLEM_LOOKUP in adapter.capabilities:
            tags = await adapter.list_problem_tags()
        elif Capability.PROBLEMSET in adapter.capabilities:
            cached = await self._problemset(platform)
            tags = self._tags_from_set(cached.items if cached else [])
        else:
            raise BadRequestError(f"平台 {platform} 不支持按算法查找")
        return ProblemTagsOut(
            items=[
                ProblemTagOut(id=t.id, name=t.name, group=t.group, count=t.count)
                for t in tags
            ]
        )

    @staticmethod
    def _tags_from_set(items: list[ProblemMeta]) -> list[TagMeta]:
        """从题单汇总标签：按出现次数降序，同次数按名称升序（结果稳定）。"""
        counter: Counter[str] = Counter()
        for meta in items:
            counter.update(meta.tags)
        return [
            TagMeta(id=name, name=name, count=count)
            for name, count in sorted(counter.items(), key=lambda kv: (-kv[1], kv[0]))
        ]

    async def review_search(
        self, platform: str, tag: str, page: int
    ) -> SearchProblemsOut:
        """按算法标签浏览一页题目，并标注 solved / collected（本地数据）。"""
        if page < 1:
            raise BadRequestError("页码必须从 1 开始")
        adapter = self._adapter(platform)
        page_size = REVIEW_SEARCH_PAGE_SIZE
        if Capability.PROBLEM_LOOKUP in adapter.capabilities:
            metas = await adapter.search_problems(tag, page=page)
        elif Capability.PROBLEMSET in adapter.capabilities:
            cached = await self._problemset(platform)
            if cached is None:
                return SearchProblemsOut(items=[], page=page, hasMore=False)
            matched = [p for p in cached.items if tag in p.tags]
            start = (page - 1) * page_size
            metas = matched[start : start + page_size]
        else:
            raise BadRequestError(f"平台 {platform} 不支持按算法查找")

        submissions = self._submissions(platform)
        solved = {s.problem_key for s in submissions if s.verdict == Verdict.AC}
        collected = {
            m.problem_key
            for m in self._store().load_manual_review()
            if m.platform == platform
        }
        return SearchProblemsOut(
            items=[
                SearchProblemOut(
                    problemKey=m.problem_key,
                    name=m.name,
                    rating=m.rating,
                    tags=list(m.tags),
                    url=m.url,
                    solved=m.problem_key in solved,
                    collected=m.problem_key in collected,
                )
                for m in metas
            ],
            page=page,
            # 末页判定：本页未满即到底（平台分页能力差异大，不依赖总数）
            hasMore=len(metas) >= page_size,
        )

    def submissions(
        self, *, date: str | None, platform: str | None
    ) -> SubmissionsOut:
        if platform is not None:
            self._adapter(platform)
        submissions = self._submissions(platform)
        tz = datetime.now().astimezone().tzinfo
        if date is not None:
            # 当日明细（按本地日期过滤）
            items = [
                s
                for s in submissions
                if datetime.fromtimestamp(s.submitted_at, tz=tz).strftime("%Y-%m-%d")
                == date
            ]
        else:
            # 近期提交：全部历史按时间倒序，取最后 RECENT_LIMIT 条
            items = sorted(
                submissions, key=lambda s: s.submitted_at, reverse=True
            )[:RECENT_LIMIT]
        return SubmissionsOut(items=[self._submission_out(s, tz) for s in items])

    @staticmethod
    def _submission_out(s: Submission, tz: tzinfo) -> SubmissionOut:
        dt = datetime.fromtimestamp(s.submitted_at, tz=tz)
        return SubmissionOut(
            id=s.submission_id,
            platform=s.platform,
            problemKey=s.problem_key,
            problemName=s.problem_name,
            problemUrl=s.problem_url,
            verdict=s.verdict,
            language=s.language,
            time=dt.strftime("%H:%M"),
            date=dt.strftime("%Y-%m-%d"),
        )

    # ===== 精细化同步（UNAC → 细分结果，§6.5） =====

    def _require_account(self, platform: str, handle: str) -> tuple[Account, PlatformAdapter]:
        """账号存在性校验并返回（账号, 适配器）。"""
        adapter = self._adapter(platform)
        profile = self._store().load_profile()
        account = next(
            (
                acc
                for acc in profile.accounts
                if acc.platform == platform and acc.handle == handle
            ),
            None,
        )
        if account is None:
            raise NotFoundError(f"账号未绑定: {platform}/{handle}")
        return account, adapter

    def start_refine(self, platform: str, handle: str) -> None:
        """启动精细化同步（后台执行；进行中重复启动 409）。"""
        _account, adapter = self._require_account(platform, handle)
        if Capability.REFINE_VERDICT not in adapter.capabilities:
            raise BadRequestError(f"平台 {platform} 不支持精细化同步")
        if not self._refine.start(self._current_group, platform, handle):
            raise ConflictError("精细化同步正在进行中")

    def stop_refine(self, platform: str, handle: str) -> None:
        """中止精细化同步（幂等；进度保留，再次启动自动续扫）。"""
        self._require_account(platform, handle)
        self._refine.stop(self._current_group, platform, handle)

    def refine_status(self, platform: str, handle: str) -> RefineStatusOut:
        """精化状态；「已完成」按存量 UNAC 清零推断（不持久化状态）。"""
        account, _adapter = self._require_account(platform, handle)
        p = self._refine.progress_of(self._current_group, platform, handle)
        store = self._store()
        items, _skipped = store.load_submissions(platform, handle)
        remaining = sum(
            1
            for s in items
            if s.verdict == Verdict.UNAC and not s.refine_attempted
        )
        if p.state.value == "running":
            state, done, total = "running", p.done, p.total
        elif p.state.value == "stopped":
            # 中止后 total 按存量剩余重算（快照分母已过时），done 保留已精化计数
            state, done, total = "stopped", p.done, remaining
        elif remaining == 0 and items:
            state, done, total = "done", p.done, max(p.total, p.done)
        else:
            state, done, total = "idle", 0, remaining
        return RefineStatusOut(state=state, done=done, total=total, auto=account.refine_auto)

    def set_refine_auto(self, platform: str, handle: str, enabled: bool) -> RefineStatusOut:
        """设置「普通同步完成后自动精化」开关（其余账号字段保持不变）。"""
        account, adapter = self._require_account(platform, handle)
        if Capability.REFINE_VERDICT not in adapter.capabilities:
            raise BadRequestError(f"平台 {platform} 不支持精细化同步")
        self._store().save_account(
            Account(
                platform=account.platform,
                handle=account.handle,
                last_synced_at=account.last_synced_at,
                display_name=account.display_name,
                last_sync_ok_at=account.last_sync_ok_at,
                sync_checkpoint=account.sync_checkpoint,
                refine_auto=enabled,
            )
        )
        return self.refine_status(platform, handle)

    # ===== 同步 =====

    async def sync(self, platform: str | None) -> None:
        if platform is not None:
            self._adapter(platform)
        store = self._store()
        profile = store.load_profile()
        targets = [
            (acc.platform, acc.handle)
            for acc in profile.accounts
            if platform is None or acc.platform == platform
        ]
        for p, h in targets:
            # 兜底包装：任何意外异常都降级为该账号诊断，不让后台任务悬空
            asyncio.create_task(self._safe_sync(p, h))

    async def sync_all_groups(self) -> None:
        """对所有用户组的全部账号触发同步（启动时用；逐组串行调度账号任务）。

        不同用户组之间共享平台限流（net.py 的 per-platform Lock），
        为避免同平台多账号并发冲击平台风控，组间采用串行调度：
        先收集全部组全部目标账号，然后逐组顺序派发，组内各账号仍并行。
        """
        groups = activity_store.list_groups(self._settings.user_data_dir)
        for group in groups:
            store = activity_store.UserStore(self._settings.user_data_dir, group)
            profile = store.load_profile()
            targets = [
                (group, acc.platform, acc.handle)
                for acc in profile.accounts
            ]
            for g, p, h in targets:
                asyncio.create_task(self._safe_sync_all_groups(g, p, h))

    async def _safe_sync_all_groups(self, user_id: str, platform: str, handle: str) -> None:
        """sync_all_groups 专用：在指定用户组上安全同步单个账号。"""
        try:
            status = await self._engine.sync_account(user_id, platform, handle)
        except Exception as exc:  # 兜底降级
            logger.exception("同步意外异常 [%s/%s/%s]", user_id, platform, handle)
            self._engine.mark_error(user_id, platform, handle, str(exc))
            return
        # 普通同步成功完成后，按账号配置自动启动精细化同步
        if status.state.value != "idle":
            return
        adapter = self._adapters.get(platform)
        if adapter is None or Capability.REFINE_VERDICT not in adapter.capabilities:
            return
        store = activity_store.UserStore(self._settings.user_data_dir, user_id)
        profile = store.load_profile()
        account = next(
            (a for a in profile.accounts if a.platform == platform and a.handle == handle),
            None,
        )
        if account is not None and account.refine_auto:
            self._refine.start(user_id, platform, handle)

    async def _safe_sync(self, platform: str, handle: str) -> None:
        try:
            status = await self._engine.sync_account(
                self._current_group, platform, handle
            )
        except Exception as exc:  # 兜底降级，见 sync()
            logger.exception("同步意外异常 [%s/%s]", platform, handle)
            self._engine.mark_error(self._current_group, platform, handle, str(exc))
            return
        # 普通同步成功完成后，按账号配置自动启动精细化同步（§6.5）
        if status.state.value != "idle":
            return
        adapter = self._adapters.get(platform)
        if adapter is None or Capability.REFINE_VERDICT not in adapter.capabilities:
            return
        profile = self._store().load_profile()
        account = next(
            (a for a in profile.accounts if a.platform == platform and a.handle == handle),
            None,
        )
        if account is not None and account.refine_auto:
            self._refine.start(self._current_group, platform, handle)

    def sync_status(self) -> list[BoundAccountOut]:
        profile = self._store().load_profile()
        return [self._account_out(acc) for acc in profile.accounts]


# 依赖注入

_service: ActivityService | None = None


def init_activity_service(settings: Settings | None = None) -> ActivityService:
    """应用启动时调用：创建服务与适配器实例。"""
    global _service
    _service = ActivityService(settings or get_settings())
    return _service


def get_activity_service() -> ActivityService:
    """FastAPI 依赖：获取全局服务实例。"""
    if _service is None:
        raise RuntimeError("ActivityService 尚未初始化")
    return _service
