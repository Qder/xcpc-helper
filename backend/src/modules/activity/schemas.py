"""activity API 出入参 DTO（对外契约）。

字段名与前端 features/activity/types.ts 对齐（camelCase），
避免 alias 转换开销；与内部存储模型（models.py）分离。
"""

from datetime import datetime
from typing import Any, Literal

from pydantic import BaseModel

from adapters.base import Capability, Verdict

SyncStateOut = Literal["idle", "running", "error"]


class BoundAccountOut(BaseModel):
    """已绑定账号 + 同步状态（platforms / sync/status 共用）。"""

    platform: str
    handle: str
    displayName: str | None = None  # 展示名（洛谷用户名等；空则前端回退 handle）
    lastSyncAt: datetime | None = None  # 最近一次成功同步的结束时间
    syncState: SyncStateOut = "idle"
    syncError: str | None = None
    syncErrorCode: str | None = None  # 结构化错误码（如 auth_expired），前端分处置路径
    syncProgress: float | None = None  # 同步进度 0~1（None = 总量未知，前端显示不定态）


class PlatformMetaOut(BaseModel):
    """平台元数据（平台页签与绑定弹窗由它驱动）。"""

    id: str
    name: str
    capabilities: list[Capability]
    auth: str
    browserLogin: bool = False  # 一键登录可用（cookie 平台 + 服务端具备 Playwright）
    homepageUrl: str = ""  # 平台主页 URL（前端跳转用）
    account: BoundAccountOut | None = None  # 该平台当前绑定账号（未绑定为 null）


class PlatformsOut(BaseModel):
    platforms: list[PlatformMetaOut]


class VerifyIn(BaseModel):
    platform: str
    handle: str
    credentials: dict[str, Any] | None = None  # cookie 授权平台验证需凭据（洛谷等）


class VerifyOut(BaseModel):
    """绑定验证回执（平台内用户基本信息）。"""

    platform: str
    handle: str
    displayName: str | None = None  # 展示名（与 API 主键分离；空回退 handle）
    avatar: str | None = None


class BindIn(BaseModel):
    platform: str
    handle: str
    displayName: str | None = None  # 验证回执带回的展示名，随绑定持久化
    credentials: dict[str, Any] | None = None  # cookie 授权平台必填（洛谷等）


class DayActivityOut(BaseModel):
    date: str  # YYYY-MM-DD（本地时区）
    submissions: int
    solved: int  # 当天 AC 的不同题目数


class OverviewTotalsOut(BaseModel):
    totalSolved: int
    totalSubmissions: int
    todaySolved: int
    weekSolved: int
    streakDays: int


class OverviewOut(BaseModel):
    totals: OverviewTotalsOut
    daily: list[DayActivityOut]  # 近约 370 天日序列，升序，末尾为今天


class SubmissionOut(BaseModel):
    """提交条目（近期提交与当日明细共用；date 字段供前端跨天合并展示）。"""

    id: str
    platform: str
    problemKey: str
    problemName: str
    problemUrl: str
    verdict: Verdict
    language: str
    time: str  # HH:mm（本地时区）
    date: str  # YYYY-MM-DD（本地时区）


class SubmissionsOut(BaseModel):
    items: list[SubmissionOut]


class SyncIn(BaseModel):
    platform: str | None = None  # 为空则同步全部账号


# ===== 错题本与同类推荐（review，设计见 activity/review.md） =====

ReviewStatusOut = Literal["unsolved", "struggled", "collected"]


class ReviewItemOut(BaseModel):
    """错题本条目（未通过 / 挣扎题 / 收录）。"""

    platform: str
    problemKey: str
    problemName: str
    problemUrl: str
    difficulty: int | str | None = None  # 平台原始难度值，不跨平台归一
    status: ReviewStatusOut
    failCount: int  # 首次 AC 之前的失败次数（unsolved 为全部失败数；收录恒 0）
    lastAttemptAt: datetime  # 最近一次尝试（本地时区；收录为加入时间）
    canRecommend: bool  # 该平台是否具备题单能力（决定展开箭头是否出现）
    manual: bool = False  # 手动收录（前端据此提供删除按钮）
    inTodo: bool = False  # 已在待办清单里（前端据此把「待办」按钮置为「已待办」）


class ReviewOut(BaseModel):
    items: list[ReviewItemOut]
    bound: bool  # 当前组是否绑定了任何账号（区分空态）
    synced: bool  # 是否至少有一个账号同步成功过


class RecommendItemOut(BaseModel):
    """同类推荐题（题单里的题目画像）。"""

    problemKey: str
    name: str
    rating: int | None = None  # 平台原始难度值（CF 为分值）
    tags: list[str]
    url: str


RecommendEmptyReason = Literal["not_indexed", "no_match"]


class RecommendOut(BaseModel):
    items: list[RecommendItemOut]
    stale: bool = False  # true = 本次用的是过期缓存（刷新失败后的降级）
    # 空结果的原因，供前端给准确提示（items 非空时为 None）：
    #   not_indexed = 源题不在平台题库/题单里（如牛客比赛专属题），无从推荐；
    #   no_match    = 源题在库内，但没有标签与难度相近的未做题。
    reason: RecommendEmptyReason | None = None


# ===== 手动收录与按算法查找 =====


class ManualReviewIn(BaseModel):
    """收录请求：{url} 或 {platform, problemKey} 二选一。

    两者都缺/都给 → 400（不猜用户意图：链接可解析出平台，题号必须带平台）。
    """

    url: str | None = None
    platform: str | None = None
    problemKey: str | None = None


class ManualReviewOut(BaseModel):
    """收录结果：created=false 表示幂等命中已有条目（不是错误）。"""

    item: ReviewItemOut
    created: bool


class ProblemTagOut(BaseModel):
    """算法标签（按算法查找的筛选项）。"""

    id: str
    name: str
    group: str = ""
    count: int | None = None


class ProblemTagsOut(BaseModel):
    items: list[ProblemTagOut]


class SearchProblemOut(BaseModel):
    """按算法搜题结果的一行。

    solved / collected 由 service 用本地数据算出，前端据此禁用按钮，
    避免前端再拉两份状态（多一次请求 + 可能不一致）。
    """

    problemKey: str
    name: str
    rating: int | None = None
    tags: list[str]
    url: str
    solved: bool = False  # 该题在提交里已 AC
    collected: bool = False  # 该题已在手动收录清单里


class SearchProblemsOut(BaseModel):
    items: list[SearchProblemOut]
    page: int
    hasMore: bool  # 是否还有下一页（按「本页是否满」判断，不依赖平台总数）


# ===== 待办题目（todo，设计见 activity/todo.md） =====


class TodoIn(BaseModel):
    """加入待办的请求体：只收定位字段，画像（题名/难度/标签）由后端补。"""

    platform: str
    problemKey: str


class TodoItemOut(BaseModel):
    """待办条目（已 AC 的条目不会出现在列表里）。"""

    platform: str
    problemKey: str
    problemName: str
    problemUrl: str
    difficulty: int | str | None = None  # 平台原始难度值，不跨平台归一
    tags: list[str]  # 加入时的标签快照（空 = 该平台无标签能力或查询失败）
    addedAt: datetime  # 加入时间（本地时区）
    status: ReviewStatusOut  # 加入时来源：未通过 / 收录（挣扎题已 AC，不会进待办）


class TodoTagOut(BaseModel):
    """标签花片：当前平台筛选下、仍未 AC 的待办里该标签出现的次数。"""

    name: str
    count: int


class TodoOut(BaseModel):
    items: list[TodoItemOut]
    tags: list[TodoTagOut]  # 花片（不受 tag 参数影响，否则无法切换标签）


class TodoAddOut(BaseModel):
    """加入结果：created=false 表示幂等命中已有条目（不是错误）。"""

    item: TodoItemOut
    created: bool



# ===== 浏览器一键登录（cookie 平台） =====

BrowserLoginStateOut = Literal["waiting", "success", "canceled", "timeout", "error"]


class BrowserLoginStatusOut(BaseModel):
    """浏览器登录会话状态（前端轮询；成功后回执同 verify）。"""

    state: BrowserLoginStateOut
    handle: str | None = None  # 成功回执：API 主键（洛谷 uid）
    displayName: str | None = None  # 成功回执：展示名
    avatar: str | None = None
    error: str | None = None  # state=error 时的诊断信息


# ===== 精细化同步（REFINE_VERDICT 能力平台，§6.5） =====

RefineStateOut = Literal["idle", "running", "stopped", "done"]


class RefineStatusOut(BaseModel):
    """精化状态：idle 时 total = 存量 UNAC 数（供前端预估耗时）。"""

    state: RefineStateOut
    done: int = 0
    total: int = 0
    auto: bool = False  # 普通同步完成后自动启动精化


class RefineConfigIn(BaseModel):
    refineAuto: bool


# ===== 用户组与信息卡 =====


class GroupOut(BaseModel):
    name: str  # 用户组名（目录名）
    current: bool


class GroupsOut(BaseModel):
    groups: list[GroupOut]


class GroupCreateIn(BaseModel):
    name: str


class GroupRenameIn(BaseModel):
    newName: str


class ProfileOut(BaseModel):
    """信息卡：显示 ID（与组名分离）/ 签名 / 头像（data URL 或 null）。"""

    id: str
    signature: str
    avatar: str | None = None


class ProfileUpdateIn(BaseModel):
    id: str | None = None
    signature: str | None = None
    avatar: str | None = None


class UpdateCredentialsIn(BaseModel):
    """更新账号凭据（仅 cookie 平台）：验证回执的 handle 必须与当前绑定一致。"""

    credentials: dict[str, Any] | None = None  # cookie 授权平台必填（一键登录时由后端消费暂存凭据）
