"""推荐打分纯函数测试：标签交集、难度窗口、排除已 AC 与源题、排序稳定性。"""

from adapters.base import ProblemMeta
from modules.activity.review import RECOMMEND_LIMIT, recommend_problems


def prob(
    key: str,
    *,
    rating: int | None = 1500,
    tags: list[str] | None = None,
    name: str = "",
) -> ProblemMeta:
    return ProblemMeta(
        problem_key=key,
        name=name or key,
        rating=rating,
        tags=tags if tags is not None else ["math"],
        url=f"https://codeforces.com/contest/{key[:4]}/problem/{key[-1]}",
    )


def test_requires_shared_tag():
    target = prob("1000A", tags=["graphs"])
    cands = [prob("1001A", tags=["math"]), prob("1002A", tags=["graphs"])]
    picked = recommend_problems(target=target, candidates=cands, solved=set())
    assert [p.problem_key for p in picked] == ["1002A"]


def test_excludes_source_and_solved():
    target = prob("1000A", tags=["math"])
    cands = [prob("1000A"), prob("1001A"), prob("1002A")]
    picked = recommend_problems(target=target, candidates=cands, solved={"1001A"})
    assert [p.problem_key for p in picked] == ["1002A"]


def test_rating_window_excludes_too_easy_and_too_hard():
    target = prob("1000A", rating=1500, tags=["math"])
    cands = [
        prob("1001A", rating=1200),  # 低于下限 200 → 排除
        prob("1002A", rating=1300),  # 恰在下限 → 保留
        prob("1003A", rating=1600),  # 恰在上限 → 保留
        prob("1004A", rating=1700),  # 超过上限 → 排除
    ]
    picked = recommend_problems(target=target, candidates=cands, solved=set())
    # 同为 1 个共同标签时按难度差升序：1600（差 100）先于 1300（差 200）
    assert [p.problem_key for p in picked] == ["1003A", "1002A"]


def test_missing_rating_falls_back_to_tag_only():
    target = prob("1000A", rating=None, tags=["math"])
    cands = [prob("1001A", rating=3500), prob("1002A", rating=800)]
    picked = recommend_problems(target=target, candidates=cands, solved=set())
    # 源题无难度：不按难度筛，两个都留（按题号稳定排序）
    assert [p.problem_key for p in picked] == ["1001A", "1002A"]


def test_candidate_missing_rating_is_kept():
    target = prob("1000A", rating=1500, tags=["math"])
    cands = [prob("1001A", rating=None)]
    assert [p.problem_key for p in recommend_problems(
        target=target, candidates=cands, solved=set()
    )] == ["1001A"]


def test_candidate_missing_rating_ranks_after_known_rating():
    """候选缺难度时排在已知难度之后（不臆断其难度合适）。"""
    target = prob("1000A", rating=1500, tags=["math"])
    cands = [
        prob("1001A", rating=None),
        prob("1002A", rating=1600),
    ]
    picked = recommend_problems(target=target, candidates=cands, solved=set())
    assert [p.problem_key for p in picked] == ["1002A", "1001A"]


def test_more_shared_tags_rank_first():
    target = prob("1000A", rating=1500, tags=["math", "dp", "greedy"])
    cands = [
        prob("1001A", rating=1500, tags=["math"]),
        prob("1002A", rating=1500, tags=["math", "dp", "greedy"]),
        prob("1003A", rating=1500, tags=["math", "dp"]),
    ]
    picked = recommend_problems(target=target, candidates=cands, solved=set())
    assert [p.problem_key for p in picked] == ["1002A", "1003A", "1001A"]


def test_closer_rating_wins_on_same_tag_count():
    target = prob("1000A", rating=1500, tags=["math"])
    cands = [
        prob("1001A", rating=1600),  # 差 100
        prob("1002A", rating=1500),  # 差 0
        prob("1003A", rating=1350),  # 差 150
    ]
    picked = recommend_problems(target=target, candidates=cands, solved=set())
    assert [p.problem_key for p in picked] == ["1002A", "1001A", "1003A"]


def test_limit_and_stable_order():
    target = prob("1000A", rating=1500, tags=["math"])
    cands = [prob(f"100{i}A", rating=1500) for i in range(1, 9)]
    picked = recommend_problems(target=target, candidates=cands, solved=set())
    assert len(picked) == RECOMMEND_LIMIT
    # 同分时按题号升序，结果稳定
    assert [p.problem_key for p in picked] == [
        "1001A",
        "1002A",
        "1003A",
        "1004A",
        "1005A",
    ]


def test_no_tags_on_target_returns_empty():
    target = prob("1000A", tags=[])
    cands = [prob("1001A", tags=["math"])]
    assert recommend_problems(target=target, candidates=cands, solved=set()) == []
