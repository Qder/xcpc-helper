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


# ===== 手动收录合并（merge_review） =====


def manual(
    key: str,
    *,
    platform: str = "codeforces",
    added_at: int = BASE_TS + 100,
    name: str = "Manual",
    difficulty: int | None = 1500,
):
    from modules.activity.models import ManualReviewItem

    return ManualReviewItem(
        platform=platform,
        problem_key=key,
        problem_name=name,
        problem_url=f"https://codeforces.com/contest/1/problem/{key}",
        difficulty=difficulty,
        added_at=added_at,
    )


def test_merge_adds_collected_items():
    """没有提交的收录项转成 collected：failCount=0、manual=True。"""
    from modules.activity.review import merge_review

    merged = merge_review([], [manual("P1001")], set())
    assert len(merged) == 1
    r = merged[0]
    assert r.status == "collected"
    assert r.fail_count == 0
    assert r.last_attempt_at == BASE_TS + 100
    assert r.first_ac_at is None
    assert r.manual is True


def test_merge_submission_takes_priority():
    """同一题既有提交聚合又有收录项 → 只保留提交那条（不重复出现）。"""
    from modules.activity.review import merge_review

    records = review_records([sub("1", "WA", key="2245A")])
    merged = merge_review(records, [manual("2245A")], set())
    assert len(merged) == 1
    assert merged[0].status == "unsolved"
    assert merged[0].manual is False


def test_merge_skips_solved_problems():
    """已在提交里 AC 的题不再显示为收录（collected 的定义就是还没过）。"""
    from modules.activity.review import merge_review

    merged = merge_review([], [manual("2245A")], {("codeforces", "2245A")})
    assert merged == []


def test_merge_sorts_by_time_desc_across_both_sources():
    """收录项与提交聚合项统一按最近时间倒序（收录时间与提交时间同口径）。"""
    from modules.activity.review import merge_review

    records = review_records([sub("1", "WA", key="OLD", ts=BASE_TS)])
    merged = merge_review(
        records,
        [manual("NEW", added_at=BASE_TS + 500), manual("MID", added_at=BASE_TS + 50)],
        set(),
    )
    assert [r.problem_key for r in merged] == ["NEW", "MID", "OLD"]


def test_merge_respects_platform_scope():
    """合并按 (platform, problem_key) 判重：同题号在不同平台各自独立。

    CF 的 A 已 AC（不进错题本），洛谷的 A 只是提交里 WA 过的一题 + 一条
    收录项；两者互不影响。
    """
    from modules.activity.review import merge_review

    records = review_records([sub("1", "WA", key="A")])
    merged = merge_review(
        records, [manual("A", platform="luogu")], {("codeforces", "A")}
    )
    by_platform = {r.platform: r for r in merged}
    assert set(by_platform) == {"codeforces", "luogu"}
    assert by_platform["codeforces"].status == "unsolved"
    assert by_platform["luogu"].status == "collected"
