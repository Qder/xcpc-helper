"""题单缓存测试：原子写入、损坏容错、TTL 判定、删除。"""

from pathlib import Path

from adapters.base import ProblemMeta
from modules.activity import problemset


def prob(key: str, rating: int = 1500, tags: list[str] | None = None) -> ProblemMeta:
    return ProblemMeta(
        problem_key=key,
        name=f"Problem {key}",
        rating=rating,
        tags=tags or ["math"],
        url=f"https://codeforces.com/contest/{key[:4]}/problem/{key[-1]}",
    )


def test_save_then_load_roundtrip(tmp_path: Path):
    items = [prob("2245A", 800, ["math"]), prob("2245B", 1600, ["greedy"])]
    assert problemset.save(tmp_path, "codeforces", items) == 2

    cached = problemset.load(tmp_path, "codeforces")
    assert cached is not None
    assert cached.fetched_at is not None
    assert cached.stale is False
    assert [p.problem_key for p in cached.items] == ["2245A", "2245B"]
    assert cached.items[0].tags == ["math"]


def test_load_missing_returns_none(tmp_path: Path):
    assert problemset.load(tmp_path, "codeforces") is None


def test_load_corrupt_json_returns_none(tmp_path: Path):
    path = problemset.paths(tmp_path, "codeforces")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("{not json", encoding="utf-8")
    assert problemset.load(tmp_path, "codeforces") is None


def test_load_skips_broken_rows(tmp_path: Path):
    """单条记录损坏只跳过，其余照常返回（诊断不阻断）。"""
    path = problemset.paths(tmp_path, "codeforces")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        '{"fetchedAt": 100, "items": ['
        '{"problem_key": "2245A", "name": "A", "tags": [], "url": ""},'
        '{"problem_key": "2245B"}'
        "]}",
        encoding="utf-8",
    )
    cached = problemset.load(tmp_path, "codeforces")
    assert cached is not None
    assert [p.problem_key for p in cached.items] == ["2245A"]
    assert cached.fetched_at == 100


def test_is_fresh_ttl():
    cached = problemset.CachedProblemset(items=[], fetched_at=1000, stale=False)
    assert problemset.is_fresh(cached, 60, now=1030) is True
    assert problemset.is_fresh(cached, 60, now=1061) is False


def test_is_fresh_without_timestamp_is_stale():
    cached = problemset.CachedProblemset(items=[], fetched_at=None, stale=False)
    assert problemset.is_fresh(cached, 3600, now=1000) is False


def test_invalidate_removes_file(tmp_path: Path):
    problemset.save(tmp_path, "codeforces", [prob("2245A")])
    assert problemset.paths(tmp_path, "codeforces").is_file()
    problemset.invalidate(tmp_path, "codeforces")
    assert not problemset.paths(tmp_path, "codeforces").exists()
    # 不存在时为空操作
    problemset.invalidate(tmp_path, "codeforces")
