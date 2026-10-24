"""洛谷 API 响应模型。

外部系统数据第一时间转化为 Pydantic 模型（见 docs/rules/backend.md）。

2026-10 洛谷前端升级为 lentille/columba 架构：`_contentOnly=1` JSON
信封下线，页面数据内嵌 HTML 的 `<script id="lentille-context">`（
`{instance, template, status, data, user, time}`）；错误态
`template == "error"`，`data` 为 `{errorCode, errorType, errorMessage}`。
record 页面（list/detail）强制登录（匿名 401 错误页）；
`api/user/search` 仍为匿名可用的裸 JSON。

容错语义：可选字段用默认值 / None 承载；类型不匹配校验失败，由 adapter
统一转为 PlatformError（平台格式异常，不阻断其他账号）。

必填字段：id（去重依据）与 submitTime（增量游标语义）——缺失即校验失败
暴露平台格式变化（submitTime 若默认 0，增量拉取会把它当作"旧于游标"
提前终止，静默丢弃后续新提交）。
"""

import re
from typing import Any

from pydantic import BaseModel, Field, field_validator

# 页面内嵌数据上下文提取（login 探针与 adapter 共用）
LENTILLE_CONTEXT_RE = re.compile(
    r'<script id="lentille-context" type="application/json">(.*?)</script>',
    re.DOTALL,
)


class LgLentilleError(BaseModel):
    """lentille 错误态 data（template == "error"）；字段缺失容忍。"""

    errorCode: int = 0
    errorType: str = ""
    errorMessage: str = ""


class LgLentilleContext(BaseModel):
    """lentille-context 页面上下文；data 按 template 由调用方二次解析。"""

    status: int = 0
    template: str = ""
    data: Any = None


class LgProblemSummary(BaseModel):
    """记录行内嵌题目信息（difficulty 0-7 档直接内嵌，无需额外请求）。

    题名字段为 name（旧 `_contentOnly` 时代为 title，新架构已改名）。
    """

    pid: str = ""
    name: str = ""
    difficulty: int | None = None


class LgContestSummary(BaseModel):
    """记录行内嵌比赛信息（非比赛内提交为 null）。"""

    id: int | None = None
    name: str = ""


class LgRecordRow(BaseModel):
    """record/list 单条记录（仅解析需要的字段，多余字段忽略）。"""

    id: int
    submitTime: int
    status: int = -1  # 缺失视为未知（-1 Unshown）→ UKE
    language: int = 0
    problem: LgProblemSummary = Field(default_factory=LgProblemSummary)
    contest: LgContestSummary | None = None
    score: int | None = None


class LgRecordPage(BaseModel):
    """records 分页容器（perPage 由服务端告知，写死会丢数据）。"""

    result: list[LgRecordRow] = Field(default_factory=list)
    count: int = 0
    perPage: int = 20


class LgRecordListData(BaseModel):
    records: LgRecordPage = Field(default_factory=LgRecordPage)


class LgUserSummary(BaseModel):
    """api/user/search 单个用户（仅解析验证需要的字段）。"""

    uid: int
    name: str = ""
    avatar: str | None = None


class LgUserSearchResult(BaseModel):
    """api/user/search 响应（裸 JSON，无信封）。"""

    users: list[LgUserSummary] = Field(default_factory=list)


# ===== 记录详情（精化用，record/:id） =====


def _normalize_nodes(data: Any) -> list[Any]:
    """subtasks / testCases 可能是数组或按编号键的 dict（d.ts 双重声明），统一为列表。"""
    if isinstance(data, dict):
        return list(data.values())
    if isinstance(data, list):
        return data
    return []


class LgTestCase(BaseModel):
    """单个测试点（status 与列表同一套官方状态码）。"""

    status: int = -1


class LgSubtask(BaseModel):
    """子任务：仅关心测试点列表（字段名为 testCases，兼容数组/dict 两种形态）。"""

    model_config = {"populate_by_name": True}

    test_cases: list[LgTestCase] = Field(default_factory=list, alias="testCases")

    @field_validator("test_cases", mode="before")
    @classmethod
    def _normalize_cases(cls, data: Any) -> list[Any]:
        return _normalize_nodes(data)


class LgJudgeResult(BaseModel):
    """评测结果：subtasks 兼容数组/dict 两种形态。"""

    subtasks: list[LgSubtask] = Field(default_factory=list)

    @field_validator("subtasks", mode="before")
    @classmethod
    def _normalize_subtasks(cls, data: Any) -> list[Any]:
        return _normalize_nodes(data)


class LgRecordDetail(BaseModel):
    """record.detail：仅取评测结果（sourceCode 等大字段忽略）。"""

    judgeResult: LgJudgeResult | None = None


class LgRecordShow(BaseModel):
    detail: LgRecordDetail | None = None


class LgRecordShowData(BaseModel):
    record: LgRecordShow = Field(default_factory=LgRecordShow)


# ===== 题目查询（PROBLEM_LOOKUP：题目画像 / 标签目录 / 按标签列表） =====


class LgProblemTag(BaseModel):
    """题目页里的标签主键（洛谷只给 id，文本要查 /_lfe/tags 的映射）。"""

    id: int


class LgProblemDetail(BaseModel):
    """题目页 data.problem（difficulty 为 0-7 档，与记录内嵌同口径）。"""

    pid: str = ""
    name: str = ""
    difficulty: int | None = None
    tags: list[int] = Field(default_factory=list)


class LgTranslationName(BaseModel):
    """translations[locale] 的最小结构（只取题名）。"""

    name: str = ""


class LgProblemShowData(BaseModel):
    """题目页 data（translations 是 locale → 翻译对象）。"""

    problem: LgProblemDetail = Field(default_factory=LgProblemDetail)
    translations: dict[str, LgTranslationName] = Field(default_factory=dict)


class LgProblemListItem(BaseModel):
    """题目列表行（/problem/list 的 result 元素）。"""

    pid: str = ""
    name: str = ""
    difficulty: int | None = None
    tags: list[int] = Field(default_factory=list)


class LgProblemListPage(BaseModel):
    """题目列表分页容器（perPage 固定 50，count 为总数）。"""

    result: list[LgProblemListItem] = Field(default_factory=list)
    count: int = 0
    perPage: int = 50


class LgProblemListData(BaseModel):
    problems: LgProblemListPage = Field(default_factory=LgProblemListPage)


class LgTagEntry(BaseModel):
    """/_lfe/tags 的标签项；type=2 才是算法标签。"""

    id: int
    name: str = ""
    type: int = 0
    parent: int | None = None


class LgTagDictionary(BaseModel):
    """/_lfe/tags 响应（裸 JSON，静态字典，含版本号）。"""

    tags: list[LgTagEntry] = Field(default_factory=list)
    _version: str = ""


