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

from adapters.base import ProblemMeta, Verdict
from modules.activity.models import ManualReviewItem, Submission

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

ReviewStatus = Literal["unsolved", "struggled", "collected"]


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
    manual: bool = False  # 来自手动收录（前端据此显示删除按钮）


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


def merge_review(
    records: Iterable[ProblemRecord],
    manual: Iterable[ManualReviewItem],
    solved: set[tuple[str, str]],
) -> list[ProblemRecord]:
    """提交聚合结果 + 手动收录清单 → 错题本列表（三类状态互斥）。

    优先级：unsolved / struggled > collected。
    - 手动项的题若已有提交聚合记录 → 跳过（提交数据是更强的证据）；
    - 手动项的题若已 AC → 跳过（collected 的定义就是「还没过」，不需要补）；
    - 其余转成 status="collected"、fail_count=0、last_attempt_at=added_at。

    solved 为 (platform, problem_key) 集合（跨该用户组全部账号）。
    """
    merged = list(records)
    known = {(r.platform, r.problem_key) for r in merged}
    for item in manual:
        key = (item.platform, item.problem_key)
        if key in known or key in solved:
            continue
        merged.append(
            ProblemRecord(
                platform=item.platform,
                problem_key=item.problem_key,
                problem_name=item.problem_name,
                problem_url=item.problem_url,
                difficulty=item.difficulty,
                status="collected",
                fail_count=0,
                last_attempt_at=item.added_at,
                first_ac_at=None,
                manual=True,
            )
        )
    merged.sort(key=lambda r: (-r.last_attempt_at, r.platform, r.problem_key))
    return merged


# ===== 同类推荐（打分与过滤，纯函数） =====

# 推荐条数上限
RECOMMEND_LIMIT = 5
# 难度窗口（相对源题）：略低到略高，避免推同题或过难的题
RATING_LOW = 200
RATING_HIGH = 100
# 候选缺难度时的排序距离：排在所有已知难度的匹配之后（不臆断其难度合适）
UNKNOWN_RATING_DISTANCE = 10**6


def recommend_problems(
    *,
    target: ProblemMeta,
    candidates: Iterable[ProblemMeta],
    solved: set[str],
    limit: int = RECOMMEND_LIMIT,
) -> list[ProblemMeta]:
    """从题单里挑同类题：至少 1 个共同 tag + 难度相近 + 未 AC 过。

    - 排除源题本身与已 AC 的题（solved 为 problem_key 集合）；
    - 源题无 rating 或候选无 rating 时，只用标签匹配（不因缺难度而全丢）；
    - 排序：共同标签数降序 → rating 差升序 → 题号（保证结果稳定）；
      候选缺 rating 时距离记为 UNKNOWN_RATING_DISTANCE，排在已知难度之后。
    """
    target_tags = set(target.tags)
    if not target_tags:
        return []

    scored: list[tuple[int, int, str, ProblemMeta]] = []
    for cand in candidates:
        if cand.problem_key == target.problem_key or cand.problem_key in solved:
            continue
        shared = len(target_tags & set(cand.tags))
        if shared == 0:
            continue
        if target.rating is not None and cand.rating is not None:
            diff = cand.rating - target.rating
            if diff < -RATING_LOW or diff > RATING_HIGH:
                continue
            distance = abs(diff)
        elif target.rating is None:
            # 源题无难度：无从比较，所有候选同权（只按标签与题号排）
            distance = 0
        else:
            # 源题有难度但候选没有：不臆断难度合适，排到已知难度之后
            distance = UNKNOWN_RATING_DISTANCE
        scored.append((-shared, distance, cand.problem_key, cand))

    scored.sort(key=lambda row: (row[0], row[1], row[2]))
    return [row[3] for row in scored[:limit]]
