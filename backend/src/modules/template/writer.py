"""content/ 目录的写入操作：可视化增删改的文件系统落盘。

设计原则：
- 所有名称在落盘前经 validate_name 校验，杜绝路径穿越与非法目录名；
- 新建版本先写入 content/ 下的 .tmp-<uuid>/ 暂存目录，
  全部写成功后一次 os.rename 到位，避免半成品被 scanner 看到；
- 更新文件走 临时文件 + os.replace 原子替换；
- 删除为物理删除（产品决策：确认弹窗已明确告知不可找回）；
- README 由 render_readme 按表单数据全量生成（表单即真相），
  写出的内容保证能被 parser 原样读回（回环一致）。
"""

import datetime
import os
import re
import shutil
import tempfile
from pathlib import Path
from typing import Any

import yaml

from common.validation import FORBIDDEN_CHARS, RESERVED_NAMES, validate_name
from core.exceptions import (
    BadRequestError,
    ConflictError,
    NotFoundError,
    PayloadTooLargeError,
)
from modules.template.models import DEFAULT_PRIORITY, MAX_EXAMPLES, is_http_url
from modules.template.scanner import (
    CODE_EXTENSIONS,
    VISUAL_FILENAME,
    VISUAL_HISTORY_DIR,
    find_visual_file,
)
from modules.template.schemas import (
    ROOT_VERSION_TOKEN,
    ExampleLink,
    VersionMetaInput,
    VersionUpsert,
)

# 名称校验规则已提升至 common.validation（打印册功能共用）
_SNAPSHOT_ID_RE = re.compile(r"^\d{8}-\d{6}Z(?:-\d+)?$")


def validate_ext(ext: str) -> str:
    """校验代码扩展名（不含点），返回小写规范化结果，非法时抛 400。"""
    cleaned = ext.strip().lower().lstrip(".")
    if f".{cleaned}" not in CODE_EXTENSIONS:
        allowed = ", ".join(sorted(e.lstrip(".") for e in CODE_EXTENSIONS))
        raise BadRequestError(f"不支持的代码扩展名: {ext!r}（支持: {allowed}）")
    return cleaned


def validate_code_filename(file: str, ext: str) -> str:
    """校验代码文件名：必须以 .<ext> 结尾，主名规则同目录名（允许内部含点）。"""
    cleaned = file.strip()
    suffix = f".{ext}"
    if not cleaned.lower().endswith(suffix):
        raise BadRequestError(f"代码文件名必须以 {suffix} 结尾: {file!r}")
    stem = cleaned[: -len(suffix)]
    if not stem or stem.startswith(".") or stem != stem.rstrip(" ."):
        raise BadRequestError(f"代码文件名不合法: {file!r}")
    bad = sorted(set(cleaned) & FORBIDDEN_CHARS)
    if bad:
        raise BadRequestError(f"代码文件名包含非法字符: {' '.join(bad)}")
    if ".." in cleaned:
        raise BadRequestError("代码文件名不能包含 '..'")
    if stem.upper() in RESERVED_NAMES:
        raise BadRequestError(f"代码文件名不能使用 Windows 保留名: {stem}")
    return cleaned


def validate_version_payload(payload: VersionUpsert) -> tuple[str, str]:
    """校验版本请求体的业务规则，返回规范化的 (扩展名, 代码文件名)。"""
    if payload.meta.page and not payload.meta.source:
        raise BadRequestError("填写了 page 但未填写 source，链接将不会显示")
    cleaned: list[ExampleLink] = []
    for item in payload.meta.examples:
        url = item.url.strip()
        if not url:
            continue
        if not is_http_url(url):
            raise BadRequestError(f"例题链接不是 http/https 地址: {item.url!r}")
        cleaned.append(ExampleLink(label=item.label.strip(), url=url))
    if len(cleaned) > MAX_EXAMPLES:
        raise BadRequestError(f"例题最多 {MAX_EXAMPLES} 条")
    payload.meta.examples = cleaned
    ext = validate_ext(payload.ext)
    filename = (
        validate_code_filename(payload.file, ext) if payload.file else f"code.{ext}"
    )
    return ext, filename


def render_readme(meta: VersionMetaInput, body: str) -> str:
    """按表单数据生成 README.md 全文（front matter + 正文）。

    字段缺省时省略对应行（与解析端"不填即默认"的语义一致）；
    priority 等于默认值时省略，保持 README 干净。
    """
    data: dict[str, Any] = {}
    if meta.updated is not None:
        data["updated"] = meta.updated.isoformat()
    tags = [t.strip() for t in meta.tags if t.strip()]
    if tags:
        data["tags"] = tags
    if meta.source and meta.source.strip():
        data["source"] = meta.source.strip()
    if meta.page and meta.page.strip():
        data["page"] = meta.page.strip()
    if meta.examples:
        data["examples"] = [
            {"label": item.label, "url": item.url} for item in meta.examples
        ]
    if meta.priority != DEFAULT_PRIORITY:
        data["priority"] = meta.priority

    # 空 front matter 也要保留一个空行（"---\n\n---"），否则解析端正则不匹配
    front = (
        yaml.safe_dump(data, allow_unicode=True, sort_keys=False) if data else "\n"
    )
    text = f"---\n{front}---\n"
    if body.strip():
        text += f"\n{body.strip()}\n"
    return text


# ===== 内部工具 =====


def _atomic_write(path: Path, text: str) -> None:
    """原子写入文本文件：先写同目录临时文件，再 os.replace 覆盖目标。"""
    fd, tmp = tempfile.mkstemp(prefix=".tmp-", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as f:
            f.write(text)
        os.replace(tmp, path)
    except BaseException:
        if os.path.exists(tmp):
            os.unlink(tmp)
        raise


def _has_visible_entries(path: Path) -> bool:
    """目录是否含有非隐藏条目（点开头的文件/目录不算，如 .gitkeep）。"""
    return any(not p.name.startswith(".") for p in path.iterdir())


def _cleanup_empty_category(content_dir: Path, category: str) -> None:
    """删除模板后顺手清理空分类目录（删不掉说明里面还有东西，忽略）。"""
    category_dir = content_dir / category
    try:
        if category_dir.is_dir() and not _has_visible_entries(category_dir):
            shutil.rmtree(category_dir)
    except OSError:
        pass


def _find_readme(path: Path) -> Path | None:
    """在目录里找 README.md（大小写不敏感），找不到返回 None。"""
    for p in path.iterdir():
        if p.is_file() and p.name.lower() == "readme.md":
            return p
    return None


# ===== 模板（主标签）操作 =====


def create_template_dir(
    content_dir: Path, category: str, name: str
) -> tuple[str, str]:
    """新建空主标签目录，返回规范化后的 (分类, 模板名)。分类不存在时一并创建。"""
    category = validate_name(category, "分类")
    name = validate_name(name, "模板")
    target = content_dir / category / name
    if target.exists():
        raise ConflictError(f"模板已存在: {category}/{name}")
    category_existed = (content_dir / category).is_dir()
    try:
        target.mkdir(parents=True, exist_ok=False)
    except FileExistsError:
        raise ConflictError(f"模板已存在: {category}/{name}") from None
    except BaseException:
        # 失败时清理刚创建的空分类目录，不留半成品
        if not category_existed:
            _cleanup_empty_category(content_dir, category)
        raise
    return category, name


def rename_template_dir(
    content_dir: Path,
    category: str,
    name: str,
    *,
    new_category: str | None = None,
    new_name: str | None = None,
) -> tuple[str, str]:
    """主标签重命名/换分类，返回新的 (分类, 模板名)。两者都不变为无操作。"""
    src = content_dir / category / name
    if not src.is_dir():
        raise NotFoundError(f"模板不存在: {category}/{name}")
    dst_category = validate_name(new_category, "分类") if new_category else category
    dst_name = validate_name(new_name, "模板") if new_name else name
    if (dst_category, dst_name) == (category, name):
        return category, name
    dst = content_dir / dst_category / dst_name
    if dst.exists():
        raise ConflictError(f"目标模板已存在: {dst_category}/{dst_name}")
    (content_dir / dst_category).mkdir(parents=True, exist_ok=True)
    os.rename(src, dst)
    _cleanup_empty_category(content_dir, category)
    return dst_category, dst_name


def resolve_version_dir(
    content_dir: Path, category: str, name: str, version_token: str
) -> Path:
    """把分类/模板/版本解析为 content_dir 内的版本目录，拒绝路径穿越。"""
    category = validate_name(category, "分类")
    name = validate_name(name, "模板")
    root = content_dir.resolve()
    if version_token == ROOT_VERSION_TOKEN:
        version_dir = (content_dir / category / name).resolve()
    else:
        slug = validate_name(version_token, "副标签")
        version_dir = (content_dir / category / name / slug).resolve()
    if not version_dir.is_relative_to(root):
        raise BadRequestError("可视化路径超出内容目录")
    return version_dir


def read_visual_bytes(
    content_dir: Path,
    category: str,
    name: str,
    version_token: str,
    max_bytes: int,
) -> bytes:
    """读取版本目录下的 visual.html；不存在 404，超限 413。"""
    version_dir = resolve_version_dir(content_dir, category, name, version_token)
    if not version_dir.is_dir():
        raise NotFoundError(f"版本不存在: {category}/{name}/{version_token}")
    visual = find_visual_file(version_dir)
    if visual is None:
        raise NotFoundError(f"可视化文件不存在: {category}/{name}/{version_token}")
    size = visual.stat().st_size
    if size > max_bytes:
        raise PayloadTooLargeError(
            f"可视化文件过大（{size} 字节，上限 {max_bytes} 字节）"
        )
    return visual.read_bytes()


def write_visual_html(
    content_dir: Path,
    category: str,
    name: str,
    version_token: str,
    html: str,
    max_bytes: int,
) -> None:
    """把 HTML 原子写入版本目录下的 visual.html。"""
    version_dir = resolve_version_dir(content_dir, category, name, version_token)
    if not version_dir.is_dir():
        raise NotFoundError(f"版本不存在: {category}/{name}/{version_token}")
    if not html.strip():
        raise BadRequestError("可视化内容不能为空")
    lowered = html.lower()
    if "<html" not in lowered and "<!doctype" not in lowered:
        raise BadRequestError("内容看起来不是 HTML（需包含 <html 或 <!doctype）")
    size = len(html.encode("utf-8"))
    if size > max_bytes:
        raise PayloadTooLargeError(
            f"可视化文件过大（{size} 字节，上限 {max_bytes} 字节）"
        )
    existing = find_visual_file(version_dir)
    if existing is not None:
        old = existing.read_text(encoding="utf-8")
        if old != html:
            _archive_visual(version_dir, old)
    target = version_dir / VISUAL_FILENAME
    if existing is not None and existing.resolve() != target.resolve():
        existing.unlink()
    _atomic_write(target, html)


def _archive_visual(version_dir: Path, html: str) -> None:
    """把将被覆盖的 visual.html 拷到 .visual-history/<stamp>.html。"""
    hist = version_dir / VISUAL_HISTORY_DIR
    hist.mkdir(exist_ok=True)
    stamp = datetime.datetime.now(datetime.UTC).strftime("%Y%m%d-%H%M%SZ")
    dest = hist / f"{stamp}.html"
    n = 2
    while dest.exists():
        dest = hist / f"{stamp}-{n}.html"
        n += 1
    _atomic_write(dest, html)


def _history_dir(content_dir: Path, category: str, name: str, version_token: str) -> Path:
    version_dir = resolve_version_dir(content_dir, category, name, version_token)
    if not version_dir.is_dir():
        raise NotFoundError(f"版本不存在: {category}/{name}/{version_token}")
    return version_dir / VISUAL_HISTORY_DIR


def _validate_snapshot_id(snapshot_id: str) -> str:
    cleaned = snapshot_id.strip()
    if not _SNAPSHOT_ID_RE.fullmatch(cleaned):
        raise BadRequestError("历史快照 id 无效")
    return cleaned


def _snapshot_path(
    content_dir: Path, category: str, name: str, version_token: str, snapshot_id: str
) -> Path:
    hist = _history_dir(content_dir, category, name, version_token)
    snap = _validate_snapshot_id(snapshot_id)
    path = (hist / f"{snap}.html").resolve()
    root = content_dir.resolve()
    if not path.is_relative_to(root):
        raise BadRequestError("可视化路径超出内容目录")
    return path


def _saved_at_from_id(snapshot_id: str) -> str:
    """把 20261020-040112Z 转成 ISO 8601；带 -2 后缀时仍用时间戳部分。"""
    day = snapshot_id[:8]
    time_part = snapshot_id[9:].split("-", 1)[0].removesuffix("Z")
    return f"{day[:4]}-{day[4:6]}-{day[6:8]}T{time_part[:2]}:{time_part[2:4]}:{time_part[4:6]}Z"


def list_visual_history(
    content_dir: Path, category: str, name: str, version_token: str
) -> list[dict[str, str | int]]:
    """列出版本目录 .visual-history/ 下的快照，新到旧。"""
    hist = _history_dir(content_dir, category, name, version_token)
    if not hist.is_dir():
        return []
    items: list[dict[str, str | int]] = []
    for path in hist.iterdir():
        if not path.is_file() or path.suffix.lower() != ".html":
            continue
        snap = path.stem
        if not _SNAPSHOT_ID_RE.fullmatch(snap):
            continue
        items.append(
            {
                "id": snap,
                "bytes": path.stat().st_size,
                "saved_at": _saved_at_from_id(snap),
            }
        )
    items.sort(key=lambda it: str(it["id"]), reverse=True)
    return items


def read_visual_history_bytes(
    content_dir: Path,
    category: str,
    name: str,
    version_token: str,
    snapshot_id: str,
    max_bytes: int,
) -> bytes:
    """读取一份历史快照；不存在 404，超限 413。"""
    path = _snapshot_path(content_dir, category, name, version_token, snapshot_id)
    if not path.is_file():
        raise NotFoundError(
            f"历史快照不存在: {category}/{name}/{version_token}/{snapshot_id}"
        )
    size = path.stat().st_size
    if size > max_bytes:
        raise PayloadTooLargeError(
            f"可视化文件过大（{size} 字节，上限 {max_bytes} 字节）"
        )
    return path.read_bytes()


def delete_visual_history_item(
    content_dir: Path, category: str, name: str, version_token: str, snapshot_id: str
) -> None:
    """删除一份历史快照，不碰当前 visual.html。"""
    path = _snapshot_path(content_dir, category, name, version_token, snapshot_id)
    if not path.is_file():
        raise NotFoundError(
            f"历史快照不存在: {category}/{name}/{version_token}/{snapshot_id}"
        )
    path.unlink()


def clear_visual_history(
    content_dir: Path, category: str, name: str, version_token: str
) -> None:
    """清空该版本全部历史快照，当前 visual.html 不动。"""
    hist = _history_dir(content_dir, category, name, version_token)
    if hist.is_dir():
        shutil.rmtree(hist)


def restore_visual_history(
    content_dir: Path,
    category: str,
    name: str,
    version_token: str,
    snapshot_id: str,
    max_bytes: int,
) -> None:
    """把快照写回当前 visual.html（现稿若不同会再归档）。"""
    data = read_visual_history_bytes(
        content_dir, category, name, version_token, snapshot_id, max_bytes
    )
    write_visual_html(
        content_dir,
        category,
        name,
        version_token,
        data.decode("utf-8"),
        max_bytes,
    )


def delete_template_dir(content_dir: Path, category: str, name: str) -> None:
    """删除空主标签目录。目录内仍有可见内容时拒绝（409），防止误删整棵树。"""
    target = content_dir / category / name
    if not target.is_dir():
        raise NotFoundError(f"模板不存在: {category}/{name}")
    if _has_visible_entries(target):
        raise ConflictError("模板目录不为空，请先删除所有版本")
    target.rmdir()
    _cleanup_empty_category(content_dir, category)


def delete_template_tree(content_dir: Path, category: str, name: str) -> None:
    """物理删除整棵模板目录树（含所有版本）。

    供导入 overwrite 与交互式批量删除使用；单条交互删除仍走
    「先删版本再删空模板」的防误删路径（delete_template_dir 拒绝非空目录）。
    """
    target = content_dir / category / name
    if not target.is_dir():
        raise NotFoundError(f"模板不存在: {category}/{name}")
    shutil.rmtree(target)
    _cleanup_empty_category(content_dir, category)


# ===== 版本操作 =====


def create_version_dir(
    content_dir: Path, category: str, name: str, payload: VersionUpsert
) -> str:
    """在模板目录下新建副标签版本目录，返回规范化后的副标签名。

    全程先在 content/ 的 .tmp-<uuid>/ 暂存目录里写好两个文件，
    再一次 rename 到位：scanner 要么看不到、要么看到完整版本。
    """
    template_dir = content_dir / category / name
    if not template_dir.is_dir():
        raise NotFoundError(f"模板不存在: {category}/{name}")
    if payload.name is None:
        raise BadRequestError("新建版本时必须填写版本名（副标签）")
    slug = validate_name(payload.name, "副标签")
    _ext, filename = validate_version_payload(payload)
    target = template_dir / slug
    if target.exists():
        raise ConflictError(f"版本已存在: {category}/{name}/{slug}")

    staging = Path(tempfile.mkdtemp(prefix=".tmp-", dir=content_dir))
    try:
        version_dir = staging / slug
        version_dir.mkdir()
        (version_dir / filename).write_text(
            payload.code, encoding="utf-8", newline=""
        )
        (version_dir / "README.md").write_text(
            render_readme(payload.meta, payload.body), encoding="utf-8", newline=""
        )
        os.rename(version_dir, target)
    finally:
        shutil.rmtree(staging, ignore_errors=True)
    return slug


def update_version_dir(
    content_dir: Path,
    category: str,
    name: str,
    current_slug: str,
    current_file: str,
    payload: VersionUpsert,
) -> str:
    """更新版本内容（代码/元数据/正文），支持副标签改名与代码文件名变更。

    current_slug 为空字符串表示"顶层单版本"（文件直接在模板目录下），
    顶层版本位置固定、不支持改名。返回更新后的副标签名。
    """
    template_dir = content_dir / category / name
    if not template_dir.is_dir():
        raise NotFoundError(f"模板不存在: {category}/{name}")
    _ext, filename = validate_version_payload(payload)

    if not current_slug:
        if payload.name and payload.name.strip():
            raise BadRequestError("顶层版本不支持改名")
        version_dir = template_dir
        new_slug = ""
    else:
        version_dir = template_dir / current_slug
        if not version_dir.is_dir():
            raise NotFoundError(f"版本不存在: {category}/{name}/{current_slug}")
        new_slug = (
            validate_name(payload.name, "副标签")
            if payload.name and payload.name.strip()
            else current_slug
        )
        if new_slug != current_slug:
            new_dir = template_dir / new_slug
            if new_dir.exists():
                raise ConflictError(f"版本已存在: {category}/{name}/{new_slug}")
            os.rename(version_dir, new_dir)
            version_dir = new_dir

    # 代码文件：文件名变化时写新文件并删除旧文件，否则原子覆盖
    _atomic_write(version_dir / filename, payload.code)
    if current_file and current_file != filename:
        old_file = version_dir / current_file
        if old_file.is_file():
            old_file.unlink()

    # README：全量重写（表单即真相）；兼容历史上小写 readme.md 的情况
    readme = _find_readme(version_dir)
    _atomic_write(
        readme or (version_dir / "README.md"),
        render_readme(payload.meta, payload.body),
    )
    return new_slug


def delete_version_dir(
    content_dir: Path, category: str, name: str, current_slug: str, current_file: str
) -> None:
    """删除一个版本：副标签版本整目录删除；顶层版本只删代码、README 与 visual.html。"""
    template_dir = content_dir / category / name
    if not template_dir.is_dir():
        raise NotFoundError(f"模板不存在: {category}/{name}")
    if current_slug:
        version_dir = template_dir / current_slug
        if not version_dir.is_dir():
            raise NotFoundError(f"版本不存在: {category}/{name}/{current_slug}")
        shutil.rmtree(version_dir)
        return
    # 顶层单版本：删除代码、README 与 visual.html，模板目录留空（成为空主标签）
    code_file = template_dir / current_file
    if code_file.is_file():
        code_file.unlink()
    readme = _find_readme(template_dir)
    if readme is not None:
        readme.unlink()
    visual = find_visual_file(template_dir)
    if visual is not None:
        visual.unlink()
    hist = template_dir / VISUAL_HISTORY_DIR
    if hist.is_dir():
        shutil.rmtree(hist)
