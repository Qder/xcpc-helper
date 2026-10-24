"""review 纯函数测试：未通过/挣扎题判定、失败口径（CE/JG 不计）、排序。"""

from adapters.base import Verdict
from modules.activity.models import Submission
from modules.activity.review import (
    STRUGGLING_FAILS,
    ProblemRecord,
    review_records,
    solved_keys,
)

BASE_TS = 1_760_000_000


def sub(
    sid: str,
    verdict: str,
    *,
    key: str = "2245A",
    platform: str = "codeforces",
    ts: int = BASE_TS,
    name: str = "Alpha",
    difficulty: int | None = 800,
) -> Submission:
    return Submission(
        platform=platform,
        handle="demo",
        submission_id=sid,
        problem_key=key,
        problem_name=name,
        problem_url=f"https://codeforces.com/contest/2245/problem/{key[-1]}",
        difficulty=difficulty,
        verdict=Verdict(verdict),
        submitted_at=ts,
        language="GNU C++17",
    )


def by_key(records: list[ProblemRecord]) -> dict[str, ProblemRecord]:
    return {r.problem_key: r for r in records}


def test_unsolved_lists_problem_without_ac():
    records = review_records([sub("1", "WA"), sub("2", "TLE", ts=BASE_TS + 10)])
    assert len(records) == 1
    r = records[0]
    assert r.status == "unsolved"
    assert r.fail_count == 2
    assert r.first_ac_at is None
    assert r.last_attempt_at == BASE_TS + 10
    assert r.problem_url.endswith("/problem/A")


def test_ac_problem_is_not_unsolved():
    records = review_records([sub("1", "WA"), sub("2", "AC", ts=BASE_TS + 10)])
    # 挣扎题阈值未到：不进错题本
    assert records == []


def test_struggled_needs_threshold_fails_before_first_ac():
    fails = [sub(str(i), "WA", ts=BASE_TS + i) for i in range(STRUGGLING_FAILS)]
    records = review_records([*fails, sub("ac", "AC", ts=BASE_TS + 100)])
    assert len(records) == 1
    r = records[0]
    assert r.status == "struggled"
    assert r.fail_count == STRUGGLING_FAILS
    assert r.first_ac_at == BASE_TS + 100


def test_fails_after_first_ac_do_not_count():
    """首次 AC 之后的失败不计入挣扎次数（已会做，不算错题）。"""
    items = [sub("ac", "AC", ts=BASE_TS)]
    items += [sub(f"f{i}", "WA", ts=BASE_TS + 10 + i) for i in range(5)]
    assert review_records(items) == []


def test_compile_error_and_judging_do_not_count_as_failures():
    items = [
        sub("1", "CE"),
        sub("2", "JG"),
        sub("3", "WA", ts=BASE_TS + 1),
        sub("4", "WA", ts=BASE_TS + 2),
        sub("ac", "AC", ts=BASE_TS + 3),
    ]
    # CE/JG 不计：首次 AC 前只有 2 次失败 < 3，不进错题本
    assert review_records(items) == []


def test_only_ce_or_judging_never_listed():
    """只有 CE/JG 的题没有"做错"证据，即使从未 AC 也不进错题本。"""
    assert review_records([sub("1", "CE"), sub("2", "JG")]) == []


def test_difficulty_falls_back_to_latest_non_null():
    items = [
        sub("1", "WA", difficulty=1500),
        sub("2", "WA", ts=BASE_TS + 10, difficulty=None),
        sub("3", "WA", ts=BASE_TS + 20, difficulty=None),
    ]
    assert review_records(items)[0].difficulty == 1500


def test_latest_submission_fields_win():
    items = [
        sub("1", "WA", name="Old", difficulty=800),
        sub("2", "WA", ts=BASE_TS + 10, name="New", difficulty=900),
    ]
    r = review_records(items)[0]
    assert r.problem_name == "New"
    assert r.difficulty == 900


def test_sorted_by_last_attempt_desc():
    items = [
        sub("1", "WA", key="A", ts=BASE_TS),
        sub("2", "WA", key="B", ts=BASE_TS + 100),
        sub("3", "WA", key="C", ts=BASE_TS + 50),
    ]
    assert [r.problem_key for r in review_records(items)] == ["B", "C", "A"]


def test_platforms_are_separate_problems():
    items = [
        sub("1", "WA", platform="codeforces"),
        sub("2", "WA", platform="atcoder"),
    ]
    records = review_records(items)
    assert len(records) == 2
    assert {r.platform for r in records} == {"codeforces", "atcoder"}


def test_solved_keys_includes_each_platform():
    items = [
        sub("1", "AC", key="A"),
        sub("2", "AC", key="A", platform="atcoder"),
        sub("3", "WA", key="B"),
    ]
    assert solved_keys(items) == {("codeforces", "A"), ("atcoder", "A")}
