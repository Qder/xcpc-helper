"""README.md 解析：front matter（YAML）+ Markdown 正文。

鲁棒性原则：任何解析问题都只产生 Diagnostic，绝不抛异常中断扫描。
"""

import datetime
import re
from pathlib import Path
from typing import Any

import yaml
from pydantic import ValidationError

from modules.template.models import (
    MAX_EXAMPLES,
    Diagnostic,
    ReadmeMeta,
    is_http_url,
)

_FRONT_MATTER_RE = re.compile(
    r"\A---[ \t]*\r?\n(.*?)\r?\n---[ \t]*\r?\n?(.*)\Z",
    re.DOTALL,
)


def _coerce_meta(data: dict[str, Any], path: str, diags: list[Diagnostic]) -> ReadmeMeta:
    """将 YAML dict 规范化为 ReadmeMeta，容忍常见的书写误差。"""
    normalized = dict(data)

    # tags 允许写成单个字符串
    tags = normalized.get("tags")
    if isinstance(tags, str):
        normalized["tags"] = [tags]
    elif tags is None:
        normalized["tags"] = []

    # priority / difficulty 允许字符串数字
    for key in ("priority", "difficulty"):
        value = normalized.get(key)
        if isinstance(value, str) and value.strip().isdigit():
            normalized[key] = int(value.strip())

    normalized["examples"] = _coerce_examples(normalized.get("examples"), path, diags)

    # updated 允许日期以外的字符串，尝试按 ISO 格式解析
    updated = normalized.get("updated")
    if isinstance(updated, str):
        try:
            normalized["updated"] = datetime.date.fromisoformat(updated.strip())
        except ValueError:
            diags.append(
                Diagnostic(level="warning", path=path, message=f"updated 日期无法解析: {updated!r}")
            )
            normalized["updated"] = None
    elif isinstance(updated, datetime.datetime):
        normalized["updated"] = updated.date()

    try:
        meta = ReadmeMeta.model_validate(normalized)
    except ValidationError as exc:
        diags.append(
            Diagnostic(level="warning", path=path, message=f"front matter 字段异常，已按默认值兜底: {exc.errors()[0]['msg']}")
        )
        meta = ReadmeMeta()

    if meta.page and not meta.source:
        diags.append(
            Diagnostic(level="warning", path=path, message="填写了 page 但未填写 source，链接将不会显示")
        )
    return meta


def _coerce_examples(
    raw: Any, path: str, diags: list[Diagnostic]
) -> list[dict[str, str]]:
    """把 examples 规范成 [{label, url}, ...]；非法项丢弃并 warning。"""
    if raw is None:
        return []
    if not isinstance(raw, list):
        diags.append(
            Diagnostic(level="warning", path=path, message="examples 不是列表，已忽略")
        )
        return []
    kept: list[dict[str, str]] = []
    dropped = 0
    for item in raw:
        if len(kept) >= MAX_EXAMPLES:
            dropped += 1
            continue
        if not isinstance(item, dict):
            dropped += 1
            continue
        url = item.get("url")
        if not isinstance(url, str) or not is_http_url(url):
            dropped += 1
            continue
        label = item.get("label")
        kept.append(
            {
                "label": label.strip() if isinstance(label, str) else "",
                "url": url.strip(),
            }
        )
    if dropped:
        diags.append(
            Diagnostic(
                level="warning",
                path=path,
                message=f"examples 中有 {dropped} 项无效或超出 {MAX_EXAMPLES} 条上限，已丢弃",
            )
        )
    return kept


def parse_readme_text(text: str, path: str, diags: list[Diagnostic]) -> tuple[ReadmeMeta, str]:
    """解析 README 文本，返回 (元数据, 正文)。正文去除首尾空白。"""
    text = text.removeprefix(chr(0xFEFF))
    match = _FRONT_MATTER_RE.match(text)
    if match is None:
        diags.append(
            Diagnostic(level="warning", path=path, message="未找到 front matter，已按默认元数据处理")
        )
        return _coerce_meta({}, path, diags), text.strip()

    raw_front, body = match.group(1), match.group(2)
    try:
        data = yaml.safe_load(raw_front)
    except yaml.YAMLError as exc:
        diags.append(Diagnostic(level="error", path=path, message=f"front matter YAML 解析失败: {exc}"))
        return _coerce_meta({}, path, diags), body.strip()

    if data is None:
        data = {}
    if not isinstance(data, dict):
        diags.append(Diagnostic(level="error", path=path, message="front matter 不是键值对结构"))
        return _coerce_meta({}, path, diags), body.strip()

    return _coerce_meta(data, path, diags), body.strip()


def parse_readme_file(file: Path, rel_path: str, diags: list[Diagnostic]) -> tuple[ReadmeMeta, str]:
    """读取并解析 README 文件，兼容 UTF-8 BOM 与 GBK 误存。"""
    try:
        text = file.read_text(encoding="utf-8-sig")
    except UnicodeDecodeError:
        diags.append(
            Diagnostic(level="warning", path=rel_path, message="文件不是 UTF-8 编码，已尝试按 GBK 读取")
        )
        text = file.read_text(encoding="gbk", errors="replace")
    except OSError as exc:
        diags.append(Diagnostic(level="error", path=rel_path, message=f"README 读取失败: {exc}"))
        return ReadmeMeta(), ""
    return parse_readme_text(text, rel_path, diags)
