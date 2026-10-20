"""模板功能的 API 请求/响应模型（对外契约）。"""

import datetime
from typing import Literal

from pydantic import BaseModel, Field

SortMode = Literal["updated", "name", "priority", "difficulty"]


class CategoryInfo(BaseModel):
    id: str  # 分类目录名
    name: str  # 显示名（目前与目录名一致，预留中文显示名扩展）
    count: int  # 该分类下的模板数


class TemplateSummary(BaseModel):
    """列表页摘要：不含代码与说明正文。"""

    id: str
    name: str
    cat: str
    lang: str | None  # 空模板（无版本）时为 None
    file: str | None  # 空模板（无版本）时为 None
    tags: list[str]
    src: str | None
    page: str | None
    updated: datetime.date | None
    priority: int
    difficulty: int
    variant_count: int
    has_visual: bool = False  # 至少一个版本存在 visual.html


class ExampleLink(BaseModel):
    """一道例题外链。label 可空，url 必须是 http/https。"""

    label: str = ""
    url: str


class TemplateVersion(BaseModel):
    """详情页中的一个版本（副标签）。body 为该版本 README 正文。"""

    id: str
    name: str
    lang: str
    file: str
    code: str
    body: str
    tags: list[str]
    src: str | None
    page: str | None
    examples: list[ExampleLink] = Field(default_factory=list)
    updated: datetime.date | None
    priority: int
    difficulty: int
    has_visual: bool = False


class TemplateDetail(TemplateSummary):
    desc: str  # 主版本 README 正文（Markdown）
    variants: list[TemplateVersion]


class DiagnosticItem(BaseModel):
    level: Literal["error", "warning"]
    path: str
    message: str


class DiagnosticsResponse(BaseModel):
    items: list[DiagnosticItem]


class ReloadResponse(BaseModel):
    templates: int
    diagnostics: int


# ===== 可视化增删改的输入模型 =====

# 寻址"顶层单版本"（代码直接在模板目录下）时 URL 中使用的保留版本名
ROOT_VERSION_TOKEN = "~"


class TemplateCreate(BaseModel):
    """新建空主标签：仅需分类与模板名。"""

    category: str
    name: str


class TemplateRename(BaseModel):
    """主标签重命名/换分类。两个字段至少填一个。"""

    new_category: str | None = None
    new_name: str | None = None


class TemplateBatchDelete(BaseModel):
    """批量删除主标签（含全部版本）。ids 为「分类/模板名」。"""

    ids: list[str]


class VersionMetaInput(BaseModel):
    """版本元数据输入：与 README front matter 一一对应。"""

    updated: datetime.date | None = None
    tags: list[str] = []
    source: str | None = None
    page: str | None = None
    examples: list[ExampleLink] = Field(default_factory=list)
    priority: int = 2
    difficulty: int = 5


class VisualUpsert(BaseModel):
    """写入某版本 visual.html 的请求体。"""

    html: str


class VisualHistoryItem(BaseModel):
    """一份 visual.html 历史快照的摘要。"""

    id: str
    bytes: int
    saved_at: str


class VisualHistoryList(BaseModel):
    items: list[VisualHistoryItem]


class VersionUpsert(BaseModel):
    """新建/更新版本的请求体。

    name：副标签名。新建时必填（一律建为副标签子目录）；
    更新时可选，用于副标签改名（顶层单版本不支持改名）。
    file：代码文件名，可选，默认为 code.<ext>。
    ext：代码扩展名（不含点，大小写不敏感），如 cpp / c / py / java。
    """

    name: str | None = None
    file: str | None = None
    ext: str
    code: str
    meta: VersionMetaInput = VersionMetaInput()
    body: str = ""
