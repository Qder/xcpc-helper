"""牛客竞赛（NowCoder）数据归一化纯函数（无 IO，便于单测）。"""

import re

from adapters.base import Verdict

# verdict 归一化：牛客状态文本 → 统一 Verdict。
# 经对 330 条真实提交全面扫描确认 8 种状态。
VERDICT_MAP: dict[str, Verdict] = {
    "正在判题": Verdict.JG,
    "答案正确": Verdict.AC,
    "答案错误": Verdict.WA,
    "运行超时": Verdict.TLE,
    "段错误": Verdict.RE,
    "内存超限": Verdict.MLE,
    "输出超限": Verdict.OLE,
    "格式错误": Verdict.WA,  # 牛客的 WA 可能是格式错误
    "内部错误": Verdict.RE,  # 牛客的 RE 可能是内部错误
    "编译错误": Verdict.CE,
    "执行出错": Verdict.RE,  # 通用运行时错误
    "浮点错误": Verdict.RE,  # SIGFPE
    "返回非零": Verdict.RE,  # 非零返回码
    "代码太长": Verdict.CE,  # 代码长度限制
}


def map_verdict(raw: str) -> Verdict:
    """牛客状态文本 → 统一 Verdict；未知结果归 UKE。"""
    return VERDICT_MAP.get(raw, Verdict.UKE)


# ===== 题目链接解析（手动收录用，纯字符串，不发请求） =====

_NOWCODER_HOST = "nowcoder.com"
# 题库题目页：/acm/problem/{纯数字}；比赛题是 /acm/contest/{cid}/problem/{数字}，
# 与题库共用同一 problem_key 口径，故一并接受。
_NC_PROBLEM_RE = re.compile(r"/acm/problem/(\d+)")
_NC_CONTEST_PROBLEM_RE = re.compile(r"/acm/contest/\d+/problem/(\d+)")
# 题霸 / 在线编程页：/practice/{32 位 uuid}。页面里没有题号，只有内部 uuid，
# problemId（题库题号）要抓页面才能拿到，故此处只识别「是不是这种链接」。
_NC_PRACTICE_RE = re.compile(r"/practice/([0-9a-zA-Z]{32})\b")


def parse_problem_url(url: str) -> str | None:
    """牛客题目链接 → problem_key（纯数字题号）；不认识返回 None。

    只认题库题（/acm/problem/{id}）与比赛题（/acm/contest/{cid}/problem/{id}）。
    返回纯数字而非 NC{id}：提交归一化用的是表格里的 problem_id（纯数字），
    用 showId 会让该题 AC 后仍显示为「收录」。

    /practice/{uuid} 这种题霸链接没有题号，需由 resolve_practice_problem_id
    抓页面换出 problemId，不在纯函数里处理。
    """
    if not url or _NOWCODER_HOST not in url.lower():
        return None
    m = _NC_PROBLEM_RE.search(url)
    if m is not None:
        return m.group(1)
    m = _NC_CONTEST_PROBLEM_RE.search(url)
    return m.group(1) if m is not None else None


def is_practice_url(url: str) -> bool:
    """是否牛客题霸 / 在线编程题链接（/practice/{uuid}，无题号）。"""
    if not url or _NOWCODER_HOST not in url.lower():
        return False
    return _NC_PRACTICE_RE.search(url) is not None


# 题霸页内嵌的 window.pageInfo 里 problemId 就是题库题号（实测与题库
# /acm/problem/list/json 的 problemId 一致）；questionId 是题霸内部 id，不能用。
_PAGE_PROBLEM_ID_RE = re.compile(r"problemId\s*:\s*['\"](\d+)['\"]")


def extract_problem_id_from_page(html: str) -> str | None:
    """题霸页面 HTML → 题库题号（problemId）；找不到返回 None。"""
    m = _PAGE_PROBLEM_ID_RE.search(html or "")
    return m.group(1) if m is not None else None
