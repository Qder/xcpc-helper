"""Codeforces API 响应模型。

外部系统数据第一时间转化为 Pydantic 模型（见 docs/rules/backend.md），
不在 adapter 内用 dict[key] 访问。CF 返回 `{"status": "OK", "result": [...]}`
信封：result 元素按端点类型化（user.info → CfUserInfo，user.status →
CfSubmissionRow）。

容错语义：可选字段用 Field 默认值 / None 承载（对应旧 dict.get 兜底，
字段缺失不炸批）；类型不匹配则校验失败，由 adapter 统一转为
PlatformError（平台格式异常，不阻断其他账号）。

必填字段：id（去重依据）与 creationTimeSeconds（增量游标语义）——
缺失即校验失败暴露平台格式变化；creationTimeSeconds 若给默认值 0，
增量拉取会把它当作"旧于游标"提前终止，静默丢弃后续新提交。
"""

from pydantic import BaseModel, Field


class CfEnvelope[T](BaseModel):
    """CF API 信封：status / comment 控制信息 + 类型化 result 列表。"""

    status: str = ""
    comment: str = ""
    result: list[T] = Field(default_factory=list)


class CfUserInfo(BaseModel):
    """user.info 单个用户（仅解析第一期需要的字段，多余字段忽略）。"""

    handle: str = ""
    avatar: str | None = None


class CfProblem(BaseModel):
    """提交行内嵌的题目信息。"""

    contestId: int | None = None
    index: str | None = None
    name: str | None = None
    rating: int | None = None


class CfSubmissionRow(BaseModel):
    """user.status 单条提交（仅解析第一期需要的字段，多余字段忽略）。"""

    id: int
    creationTimeSeconds: int
    problem: CfProblem | None = None
    programmingLanguage: str = ""
    verdict: str = ""


class CfProblemsetProblem(BaseModel):
    """problemset.problems 中的一道题（题目画像，含标签）。"""

    contestId: int | None = None
    index: str | None = None
    name: str | None = None
    rating: int | None = None
    tags: list[str] = Field(default_factory=list)


class CfProblemsetResult(BaseModel):
    """problemset.problems 的 result。

    注意与 user.status 不同：外层 result 是**对象**（含 problems 与
    problemStatistics 两个列表），不是列表，故不能复用 CfEnvelope[list]。
    只需 problems，problemStatistics 忽略。
    """

    problems: list[CfProblemsetProblem] = Field(default_factory=list)


class CfProblemsetEnvelope(BaseModel):
    """题单信封：result 为对象（独立模型，不复用列表信封的 result 定义）。"""

    status: str = ""
    comment: str = ""
    result: CfProblemsetResult = Field(default_factory=CfProblemsetResult)
