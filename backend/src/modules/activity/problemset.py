"""题单缓存（推荐用，可删除重建）。

设计见 docs/design/activity/review.md §3.2：
- 属于缓存而非事实来源，落在 data/cache/activity/problemset/<platform>.json（不入 git）；
- TTL 过期后在下一次推荐请求时刷新，不在启动时预拉（不拖慢启动）；
- 刷新失败保留旧缓存继续服务（stale=true），无旧缓存则返回 None 由服务层降级；
- 原子写入沿用 store 的临时文件 + os.replace 模式。
"""

import json
import os
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

from adapters.base import ProblemMeta

CACHE_DIR = Path("cache") / "activity" / "problemset"

# 嵌入缓存的额外元数据（与题目列表分开存，避免与 ProblemMeta 字段混淆）
META_FETCHED_AT = "fetchedAt"


@dataclass(frozen=True, slots=True)
class CachedProblemset:
    """一份题单缓存。

    stale=True 表示本次返回的是过期缓存（刷新失败后的降级）；
    fetched_at 为缓存写入时刻（UTC 秒），未知时为 None。
    """

    items: list[ProblemMeta]
    fetched_at: int | None
    stale: bool


def _cache_file(data_dir: Path, platform: str) -> Path:
    return data_dir / CACHE_DIR / f"{platform}.json"


def _atomic_write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix=".tmp-", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as f:
            f.write(text)
        os.replace(tmp, path)
    except BaseException:
        if os.path.exists(tmp):
            os.unlink(tmp)
        raise


def is_fresh(cached: CachedProblemset, ttl_seconds: int, *, now: int | None = None) -> bool:
    """缓存是否仍在 TTL 内（无 fetched_at 视为过期，需要刷新）。"""
    if cached.fetched_at is None:
        return False
    ts = now if now is not None else int(time.time())
    return ts - cached.fetched_at < ttl_seconds


def load(data_dir: Path, platform: str) -> CachedProblemset | None:
    """读取缓存；不存在或损坏返回 None（缓存可随时删除重建，不阻断）。"""
    path = _cache_file(data_dir, platform)
    if not path.is_file():
        return None
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None
    if not isinstance(raw, dict):
        return None
    items_raw = raw.get("items")
    if not isinstance(items_raw, list):
        return None
    items: list[ProblemMeta] = []
    for row in items_raw:
        try:
            items.append(ProblemMeta.model_validate(row))
        except ValueError:
            # 单条损坏只跳过（诊断不阻断）
            continue
    fetched_at = raw.get(META_FETCHED_AT)
    return CachedProblemset(
        items=items,
        fetched_at=fetched_at if isinstance(fetched_at, int) else None,
        stale=False,
    )


def save(data_dir: Path, platform: str, items: list[ProblemMeta]) -> int:
    """原子写入题单缓存，返回写入的题目数。"""
    fetched_at = int(time.time())
    payload = {
        META_FETCHED_AT: fetched_at,
        "items": [item.model_dump(mode="json") for item in items],
    }
    text = json.dumps(payload, ensure_ascii=False, indent=2)
    _atomic_write(_cache_file(data_dir, platform), text)
    return len(items)


def invalidate(data_dir: Path, platform: str) -> None:
    """删除指定平台的题单缓存（不存在为空操作）。"""
    _cache_file(data_dir, platform).unlink(missing_ok=True)


def paths(data_dir: Path, platform: str) -> Path:
    """缓存文件路径（测试与诊断用）。"""
    return _cache_file(data_dir, platform)


__all__ = [
    "CACHE_DIR",
    "CachedProblemset",
    "invalidate",
    "is_fresh",
    "load",
    "paths",
    "save",
]
