"""模板功能的内部领域模型（扫描/解析产物）。

这些模型描述 content/ 目录被解析后的内存结构，
与 API 对外契约（schemas.py）和索引存储（repository.py）分离。
"""

import datetime
from typing import Literal
from urllib.parse import urlparse

from pydantic import BaseModel, ConfigDict, Field

DEFAULT_PRIORITY = 2
DEFAULT_DIFFICULTY = 5
MAX_EXAMPLES = 12


def is_http_url(url: str) -> bool:
    """仅允许带主机名的 http/https 绝对地址。"""
    try:
        parsed = urlparse(url.strip())
    except ValueError:
        return False
    return parsed.scheme in ("http", "https") and bool(parsed.netloc)


class ExampleLink(BaseModel):
    """一道例题外链。label 可空，url 必须是 http/https。"""

    label: str = ""
    url: str


class ReadmeMeta(BaseModel):
    """README.md front matter 元数据。
    显示名取自目录名，无需 title 字段；允许规范之外的额外字段，保证向前兼容。
    """

    model_config = ConfigDict(extra="allow")

    updated: datetime.date | None = None
    tags: list[str] = []
    source: str | None = None
    page: str | None = None
    examples: list[ExampleLink] = Field(default_factory=list)
    priority: int = DEFAULT_PRIORITY
    difficulty: int = DEFAULT_DIFFICULTY


class VersionNode(BaseModel):
    """一个模板版本（副标签）。单版本模板同样用一个 VersionNode 表示。"""

    slug: str  # 副标签目录名；单版本（文件直接在模板目录下）时为空字符串
    name: str  # 显示名：副标签目录名，单版本时为模板目录名
    lang: str
    file: str
    code: str
    meta: ReadmeMeta
    body: str  # README 正文（front matter 之后的 Markdown）
    has_visual: bool = False  # 版本目录是否存在 visual.html（大小写不敏感）


class TemplateNode(BaseModel):
    """一份模板（主标签），id 为 "<分类目录名>/<模板目录名>"。"""

    id: str
    category: str
    slug: str  # 模板目录名（主标签）
    versions: list[VersionNode]


class Diagnostic(BaseModel):
    """扫描诊断：格式问题不阻断整体加载，逐条上报给前端。"""

    level: Literal["error", "warning"]
    path: str  # 相对 content/ 的路径
    message: str


class ScanResult(BaseModel):
    templates: list[TemplateNode]
    diagnostics: list[Diagnostic]
