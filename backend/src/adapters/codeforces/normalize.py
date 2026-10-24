"""Codeforces 数据归一化纯函数（无 IO，便于单测）。"""

import re

from adapters.base import Verdict

# verdict 归一化：OK→AC 等；未列出的（CHALLENGED / SKIPPED / PARTIAL /
# FAILED / CRASHED / IDLENESS_LIMIT_EXCEEDED / INPUT_PREPARATION_CRASHED 等）
# 一律归 UKE；SUBMITTED / TESTING 归 JG（评测中）。
VERDICT_MAP: dict[str, Verdict] = {
    "OK": Verdict.AC,
    "WRONG_ANSWER": Verdict.WA,
    "COMPILATION_ERROR": Verdict.CE,
    "RUNTIME_ERROR": Verdict.RE,
    "TIME_LIMIT_EXCEEDED": Verdict.TLE,
    "MEMORY_LIMIT_EXCEEDED": Verdict.MLE,
    "SUBMITTED": Verdict.JG,
    "TESTING": Verdict.JG,
}


def map_verdict(raw: str) -> Verdict:
    """CF verdict 文本 → 统一 Verdict；未知结果归 UKE。"""
    return VERDICT_MAP.get(raw, Verdict.UKE)


def problem_url(contest_id: int | None, index: str | None) -> str:
    """题目外链；缺 contestId / index 时兜底平台主页。

    CF 主题库与 gym 题库的 URL 形式不同，按 contest_id 位数区分：
    主题库一般为四位数（/contest/ 页），gym 一般为六位数（/gym/ 页）。
    """
    if contest_id is not None and index:
        if contest_id >= 100000:
            return f"https://codeforces.com/gym/{contest_id}/problem/{index}"
        return f"https://codeforces.com/contest/{contest_id}/problem/{index}"
    return "https://codeforces.com"


def problem_key(contest_id: int | None, index: str | None, name: str) -> str:
    """平台内题目标识：contestId + index（如 2245F）；缺失时退化为题名。"""
    if contest_id is not None and index:
        return f"{contest_id}{index}"
    return name or "?"


# ===== 题目链接解析（手动收录用，纯字符串，不发请求） =====

# 官方域（镜像站/移动端/无域名前缀的裸路径都不认，避免把别站题号当 CF 题）
_CF_HOST = "codeforces.com"
# /contest/{id}/problem/{index}、/gym/{id}/problem/{index}
# （group 链接 /group/{hash}/contest/{id}/problem/{index} 也含这一段，一并命中）
_CF_PATH_RE = re.compile(r"/(?:contest|gym)/(\d+)/problem/([A-Za-z0-9]+)", re.IGNORECASE)
# /problemset/problem/{id}/{index}
_CF_PROBLEMSET_RE = re.compile(
    r"/problemset/problem/(\d+)/([A-Za-z0-9]+)", re.IGNORECASE
)


def parse_problem_url(url: str) -> str | None:
    """Codeforces 题目链接 → problem_key（如 2245F）；不认识返回 None。

    识别三种形态（忽略 scheme / query / hash / 尾斜杠 / 大小写差异）：
    - /contest/2245/problem/F
    - /gym/105234/problem/A
    - /problemset/problem/2245/F
    与问题的 problem_key 同口径（contestId + index 直接拼接），
    否则「收录后 AC 了仍显示为收录」。
    """
    if not url or _CF_HOST not in url.lower():
        return None
    m = _CF_PATH_RE.search(url)
    if m is None:
        m = _CF_PROBLEMSET_RE.search(url)
    if m is None:
        return None
    return f"{m.group(1)}{m.group(2).upper()}"
