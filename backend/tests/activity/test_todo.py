"""待办题目（todo）服务层测试：加入 / 已 AC 拒绝 / 幂等 / 标签快照 / 花片 / 移出。

设计见 docs/design/activity/todo.md。全部走本地 MockTransport，不打真实平台。
"""

import time
from unittest.mock import AsyncMock

import httpx
import pytest

from adapters.base import Capability, PlatformError, ProblemMeta, TagMeta
from adapters.net import HttpFetcher
from core.config import Settings
from core.exceptions import BadRequestError, NotFoundError
from modules.activity import problemset
from modules.activity.models import Account, Submission, Verdict
from modules.activity.schemas import ManualReviewIn, TodoIn
from services.activity.service import ActivityService


def make_todo_service(tmp_path, handler=None) -> ActivityService:
    """待办测试用服务：用户目录与缓存目录都指向 tmp，避免污染真实 data/。"""
    fetcher = HttpFetcher(
        transport=httpx.MockTransport(handler or (lambda r: httpx.Response(404))),
        base_backoff=0.01,
    )
    svc = ActivityService(
        Settings(user_data_dir=tmp_path / "user", data_dir=tmp_path / "data"),
        fetcher,
    )
    svc._adapters["nowcoder"].min_interval = 0
    return svc


def cache_set(svc: ActivityService, items: list[ProblemMeta]) -> None:
    """把题单缓存替换成固定内容（CF 的标签来源，零网络）。"""
    svc._problemset = AsyncMock(  # type: ignore[method-assign]
        return_value=problemset.CachedProblemset(
            items=items, fetched_at=int(time.time()), stale=False
        )
    )


def add_submission(
    svc: ActivityService, key: str, verdict: Verdict = Verdict.WA
) -> None:
    store = svc._store()
    store.save_account(Account(platform="codeforces", handle="demo"))
    store.merge_submissions(
        "codeforces",
        "demo",
        [
            Submission(
                platform="codeforces",
                handle="demo",
                submission_id=f"s-{key}-{verdict.value}",
                problem_key=key,
                problem_name=key,
                problem_url=f"https://codeforces.com/contest/2245/problem/{key[-1]}",
                verdict=verdict,
                submitted_at=int(time.time()),
                language="GNU C++17",
            )
        ],
    )


async def test_todo_add_snapshots_tags_and_status(tmp_path):
    """加入待办：题名/难度/标签来自画像，来源状态由提交决定。"""
    svc = make_todo_service(tmp_path)
    cache_set(
        svc, [ProblemMeta(problem_key="2245F", name="Xor", rating=2100, tags=["dp"])]
    )
    try:
        add_submission(svc, "2245F", Verdict.WA)
        out = await svc.add_todo(TodoIn(platform="codeforces", problemKey="2245F"))
        assert out.created is True
        assert out.item.problemName == "Xor"
        assert out.item.difficulty == 2100
        assert out.item.tags == ["dp"]
        assert out.item.status == "unsolved"
    finally:
        await svc.aclose()


async def test_todo_add_without_submission_is_collected(tmp_path):
    """没有提交记录的题（收录来源）→ status=collected。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        out = await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000A"))
        assert out.item.status == "collected"
        assert out.item.problemUrl == "https://codeforces.com/problemset/problem/1000A"
    finally:
        await svc.aclose()


async def test_todo_add_rejects_ac_problem(tmp_path):
    """已 AC 的题不能加入待办（接口层挡住，不靠前端隐藏入口）。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        add_submission(svc, "2245F", Verdict.AC)
        with pytest.raises(BadRequestError) as exc:
            await svc.add_todo(TodoIn(platform="codeforces", problemKey="2245F"))
        assert "已 AC" in str(exc.value)
        assert svc._store().load_todo() == []
    finally:
        await svc.aclose()


async def test_todo_add_is_idempotent(tmp_path):
    """重复加入同一题：created=False，不产生第二条，addedAt 保留首次。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        first = await svc.add_todo(TodoIn(platform="codeforces", problemKey="2245F"))
        second = await svc.add_todo(TodoIn(platform="codeforces", problemKey="2245F"))
        assert first.created is True
        assert second.created is False
        assert second.item.addedAt == first.item.addedAt
        assert len(svc._store().load_todo()) == 1
    finally:
        await svc.aclose()


async def test_todo_add_rejects_unknown_platform_and_empty_key(tmp_path):
    svc = make_todo_service(tmp_path)
    try:
        with pytest.raises(BadRequestError):
            await svc.add_todo(TodoIn(platform="nope", problemKey="1"))
        with pytest.raises(BadRequestError):
            await svc.add_todo(TodoIn(platform="codeforces", problemKey="  "))
    finally:
        await svc.aclose()


async def test_todo_add_survives_profile_failure(tmp_path):
    """画像拿不到（离线）：仍允许加入，题名退化为题号、标签为空。"""
    svc = make_todo_service(tmp_path)
    try:
        out = await svc.add_todo(TodoIn(platform="codeforces", problemKey="2245F"))
        assert out.created is True
        assert out.item.problemName == "2245F"
        assert out.item.tags == []
        assert out.item.difficulty is None
    finally:
        await svc.aclose()


def make_stub_luogu(*, tags: list[TagMeta] | None = None, raise_on_tags=False):
    """桩 adapter 顶替洛谷（它走自己的 curl_cffi 会话，不受 MockTransport 影响）。"""

    class StubLuogu:
        platform_id = "luogu"

        def __init__(self) -> None:
            self.capabilities = {Capability.PROBLEM_LOOKUP}

        async def fetch_problem(self, problem_key: str) -> ProblemMeta:
            return ProblemMeta(
                problem_key=problem_key,
                name="A+B Problem",
                rating=1,
                tags=[],  # 题目页只有主键
                tag_ids=["1", "2"],
                url="https://www.luogu.com.cn/problem/P1001",
            )

        async def list_problem_tags(self) -> list[TagMeta]:
            if raise_on_tags:
                raise PlatformError("洛谷返回 HTTP 403")
            return tags or []

    return StubLuogu()


async def test_todo_add_resolves_tag_text_from_dictionary(tmp_path):
    """题目页只给标签主键时，加入时查一次标签字典映射成名称（洛谷路径）。"""
    svc = make_todo_service(tmp_path)
    svc._adapters["luogu"] = make_stub_luogu(  # type: ignore[assignment]
        tags=[
            TagMeta(id="1", name="模拟"),
            TagMeta(id="2", name="数学"),
            TagMeta(id="3", name="洛谷原创"),
        ]
    )
    try:
        out = await svc.add_todo(TodoIn(platform="luogu", problemKey="P1001"))
        assert out.item.tags == ["模拟", "数学"]
        assert out.item.problemName == "A+B Problem"
    finally:
        await svc.aclose()


async def test_todo_add_keeps_tags_empty_when_dictionary_unavailable(tmp_path):
    """标签字典拉不到（离线 / 反爬）：加入依旧成功，只是没有花片。"""
    svc = make_todo_service(tmp_path)
    svc._adapters["luogu"] = make_stub_luogu(raise_on_tags=True)  # type: ignore[assignment]
    try:
        out = await svc.add_todo(TodoIn(platform="luogu", problemKey="P1001"))
        assert out.created is True
        assert out.item.tags == []
    finally:
        await svc.aclose()


async def test_todo_list_hides_ac_and_sorts_by_added_at(tmp_path):
    """已 AC 的待办从列表隐藏（磁盘仍在）；列表按 addedAt 倒序。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000A"))
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000B"))
        add_submission(svc, "1000A", Verdict.AC)

        out = svc.todo_list(None, None)
        assert [i.problemKey for i in out.items] == ["1000B"]
        # 已 AC 的条目不被删除，同步把提交冲掉后还能回来
        assert {it.problem_key for it in svc._store().load_todo()} == {
            "1000A",
            "1000B",
        }
    finally:
        await svc.aclose()


async def test_todo_list_platform_filter_and_unknown_platform(tmp_path):
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000A"))
        assert svc.todo_list("codeforces", None).items[0].problemKey == "1000A"
        assert svc.todo_list("nowcoder", None).items == []
        with pytest.raises(BadRequestError):
            svc.todo_list("nope", None)
    finally:
        await svc.aclose()


async def test_todo_tag_chips_ignore_tag_filter(tmp_path):
    """花片只受「已 AC + 平台」影响，不受 tag 参数影响（否则无法切换标签）。"""
    svc = make_todo_service(tmp_path)
    cache_set(
        svc,
        [
            ProblemMeta(problem_key="1000A", name="a", tags=["dp", "math"]),
            ProblemMeta(problem_key="1000B", name="b", tags=["dp"]),
        ],
    )
    try:
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000A"))
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000B"))

        out = svc.todo_list(None, None)
        assert [(t.name, t.count) for t in out.tags] == [("dp", 2), ("math", 1)]

        filtered = svc.todo_list(None, "math")
        assert [i.problemKey for i in filtered.items] == ["1000A"]
        # 花片仍完整
        assert [(t.name, t.count) for t in filtered.tags] == [
            ("dp", 2),
            ("math", 1),
        ]
    finally:
        await svc.aclose()


async def test_todo_tag_filter_is_exact_match(tmp_path):
    """标签筛选是精确匹配，不做子串（「dp」不应命中「线性dp」）。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [ProblemMeta(problem_key="1000A", name="a", tags=["线性dp"])])
    try:
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000A"))
        assert svc.todo_list(None, "dp").items == []
        assert len(svc.todo_list(None, "线性dp").items) == 1
    finally:
        await svc.aclose()


async def test_todo_remove(tmp_path):
    """移出待办：删到即成功；不存在 404；不影响错题本收录清单。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000A"))
        svc.remove_todo("codeforces", "1000A")
        assert svc.todo_list(None, None).items == []
        with pytest.raises(NotFoundError):
            svc.remove_todo("codeforces", "1000A")
    finally:
        await svc.aclose()


async def test_review_marks_in_todo(tmp_path):
    """错题本每条带 inTodo，前端据此刻画「待办 / 已待办」按钮。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        add_submission(svc, "2245B", Verdict.WA)
        add_submission(svc, "2245D", Verdict.WA)

        assert all(not i.inTodo for i in svc.review_list(None, "all").items)

        await svc.add_todo(TodoIn(platform="codeforces", problemKey="2245B"))
        by_key = {i.problemKey: i for i in svc.review_list(None, "all").items}
        assert by_key["2245B"].inTodo is True
        assert by_key["2245D"].inTodo is False
    finally:
        await svc.aclose()


async def test_manual_review_marks_in_todo(tmp_path):
    """收录接口的回执也带 inTodo（前端本地并入列表时不丢按钮状态）。"""
    svc = make_todo_service(tmp_path)
    cache_set(svc, [])
    try:
        await svc.add_todo(TodoIn(platform="codeforces", problemKey="1000A"))
        out = await svc.add_manual_review(
            ManualReviewIn(platform="codeforces", problemKey="1000A")
        )
        assert out.item.inTodo is True
    finally:
        await svc.aclose()
