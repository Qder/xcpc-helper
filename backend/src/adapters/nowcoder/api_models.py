"""牛客 API 响应模型。

牛客 practice-coding 页面为服务端渲染 HTML，非 JSON。
本模块定义 HTML 解析结果的 Pydantic 模型，用于校验提取后的数据。
"""

from pydantic import BaseModel, Field


class NcSubmissionRow(BaseModel):
    """practice-coding 单条提交（解析 HTML 表格行后校验）。"""

    submission_id: str
    problem_id: str
    problem_name: str
    status_text: str
    language: str
    submitted_at_str: str  # YYYY-MM-DD HH:MM:SS（中国时区）


class NcRatingHistoryEntry(BaseModel):
    """rating-history 单条记录（仅用于验证接口回执结构）。"""

    contestId: int
    rating: float
    rank: int
    changeValue: float
    time: int  # ms
    contestName: str
    colorLevel: int


class NcRatingHistoryEnvelope(BaseModel):
    """rating-history 响应信封。"""

    msg: str = ""
    code: int = 0
    data: list[NcRatingHistoryEntry] = Field(default_factory=list)


# ===== 题库 JSON 接口（按算法查找 / 定点查询用） =====


class NcProblemTag(BaseModel):
    """题目行内的知识点标签（id 是题库 tagId，name 是展示名）。"""

    id: int
    name: str = ""
    parentId: int | None = None


class NcProblemRow(BaseModel):
    """list/json 单条题目。

    problemId 是纯数字题号（与提交归一化的 problem_key 同口径），
    showId 是 NC{problemId} 的展示型题号——入库一律用 problemId。
    """

    problemId: int
    showId: str = ""
    name: str = ""
    difficulty: int | None = None
    tagList: list[NcProblemTag] = Field(default_factory=list)


class NcProblemListData(BaseModel):
    problemCount: int = 0
    problemSets: list[NcProblemRow] = Field(default_factory=list)


class NcProblemListEnvelope(BaseModel):
    code: int = 0
    msg: str = ""
    data: NcProblemListData | None = None


class NcKnowledgeTag(BaseModel):
    """知识点目录的叶子标签（可直接当 tagId 用于题库过滤）。"""

    id: int
    name: str = ""
    groupName: str = ""
    totalCount: int | None = None


class NcKnowledgeGroup(BaseModel):
    id: int
    name: str = ""
    hidden: bool = False
    childTags: list[NcKnowledgeTag] = Field(default_factory=list)


class NcKnowledgeCatalogData(BaseModel):
    groupCount: int = 0
    groups: list[NcKnowledgeGroup] = Field(default_factory=list)


class NcKnowledgeCatalogEnvelope(BaseModel):
    code: int = 0
    msg: str = ""
    data: NcKnowledgeCatalogData | None = None
