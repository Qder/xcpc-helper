"""纯函数：submissions → 错题本条目（无 IO，便于单测）。

统计口径见 docs/design/activity/review.md「产品口径」：
- 题目身份 = (platform, problem_key)，不做跨平台同题合并；
- 未通过（unsolved）= 全部提交中从未 AC；
- 挣扎题（struggled）= 有过 AC，且首次 AC 之前的失败次数 ≥ STRUGGLING_FAILS；
- 计入失败 = WA/TLE/MLE/RE/OLE/UNAC/UKE；CE（编译错误）与 JG（评测中）不计。
"""

from collections.abc import Iterable
from dataclasses import dataclass
from typing import Literal

from adapters.base import Verdict
from modules.activity.models import Submission

# 挣扎题阈值：首次 AC 之前的失败提交数达到该值即视为"做了很多次才过"
STRUGGLING_FAILS = 3

# 计入失败的判定（CE 属本地环境问题、JG 属评测中，均不代表做错）
FAIL_VERDICTS: frozenset[Verdict] = frozenset(
    {
        Verdict.WA,
        Verdict.TLE,
        Verdict.MLE,
        Verdict.RE,
        Verdict.OLE,
        Verdict.UNAC,
        Verdict.UKE,
    }
)

ReviewStatus = Literal["unsolved", "struggled"]


@dataclass(frozen=True, slots=True)
class ProblemRecord:
    """单道题在全部提交中的汇总（错题本一行）。"""

    platform: str
    problem_key: str
    problem_name: str
    problem_url: str
    difficulty: int | str | None
    status: ReviewStatus
    fail_count: int
    last_attempt_at: int
    first_ac_at: int | None


def _group_key(s: Submission) -> tuple[str, str]:
    return (s.platform, s.problem_key)


def review_records(submissions: Iterable[Submission]) -> list[ProblemRecord]:
    """提交 → 错题条目（未通过 + 挣扎题），按最近尝试时间倒序。

    同一题目多条提交取最近一条的题名/外链；难度取最近的非空值
    （平台对旧记录可能不再返回难度）。
    """
    grouped: dict[tuple[str, str], list[Submission]] = {}
    for s in submissions:
        grouped.setdefault(_group_key(s), []).append(s)

    out: list[ProblemRecord] = []
    for (platform, key), items in grouped.items():
        ordered = sorted(items, key=lambda s: (s.submitted_at, s.submission_id))
        ac_at = next(
            (s.submitted_at for s in ordered if s.verdict == Verdict.AC), None
        )
        if ac_at is None:
            status: ReviewStatus = "unsolved"
            # 未通过：全部失败提交都算（CE/JG 仍不计）
            fail_count = sum(1 for s in ordered if s.verdict in FAIL_VERDICTS)
            if fail_count == 0:
                # 只有 CE / JG / 未知：没有"做错"的证据，不进错题本
                continue
        else:
            status = "struggled"
            fail_count = sum(
                1
                for s in ordered
                if s.verdict in FAIL_VERDICTS and s.submitted_at < ac_at
            )
            if fail_count < STRUGGLING_FAILS:
                continue

        latest = ordered[-1]
        # 难度取最近的非空值（时间倒序找第一个非空）
        difficulty = next(
            (
                s.difficulty
                for s in reversed(ordered)
                if s.difficulty is not None
            ),
            None,
        )
        out.append(
            ProblemRecord(
                platform=platform,
                problem_key=key,
                problem_name=latest.problem_name,
                problem_url=latest.problem_url,
                difficulty=difficulty,
                status=status,
                fail_count=fail_count,
                last_attempt_at=latest.submitted_at,
                first_ac_at=ac_at,
            )
        )

    out.sort(key=lambda r: (-r.last_attempt_at, r.platform, r.problem_key))
    return out


def solved_keys(submissions: Iterable[Submission]) -> set[tuple[str, str]]:
    """已 AC 的题目集合（推荐候选排除用）。"""
    return {_group_key(s) for s in submissions if s.verdict == Verdict.AC}
