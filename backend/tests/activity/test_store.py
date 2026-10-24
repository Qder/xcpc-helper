"""UserStore 读写层测试：profile 往返、原子写、去重合并、损坏容错、名称校验、用户组目录管理。"""

import json

import pytest

from adapters.base import Credentials
from core.exceptions import BadRequestError, ConflictError, NotFoundError
from modules.activity.models import Account, Submission, TodoItem, Verdict
from modules.activity.store import (
    EXAMPLE_GROUP,
    UserStore,
    create_group,
    delete_group,
    list_groups,
    rename_group,
)


def make_store(tmp_path) -> UserStore:
    return UserStore(tmp_path / "user", "default")


def submission(sid: str, ts: int, verdict: Verdict = Verdict.AC) -> Submission:
    return Submission(
        platform="codeforces",
        handle="demo",
        submission_id=sid,
        problem_key="2245A",
        problem_name="X Axis",
        problem_url="https://codeforces.com/contest/2245/problem/A",
        verdict=verdict,
        submitted_at=ts,
        language="GNU C++17",
    )


def test_profile_roundtrip(tmp_path):
    store = make_store(tmp_path)
    profile = store.load_profile()
    assert profile.id == "default"
    assert profile.accounts == []

    profile.accounts.append(Account(platform="codeforces", handle="demo"))
    store.save_profile(profile)

    loaded = store.load_profile()
    assert loaded.id == "default"
    assert loaded.accounts[0].handle == "demo"


def test_save_account_add_and_update(tmp_path):
    store = make_store(tmp_path)
    store.save_account(Account(platform="codeforces", handle="demo"))
    store.save_account(Account(platform="codeforces", handle="demo", last_synced_at=1000))

    profile = store.load_profile()
    assert len(profile.accounts) == 1
    assert profile.accounts[0].last_synced_at == 1000


def test_remove_account_deletes_file(tmp_path):
    store = make_store(tmp_path)
    store.save_account(Account(platform="codeforces", handle="demo"))
    store.merge_submissions("codeforces", "demo", [submission("1", 1000)])
    assert store.load_submissions("codeforces", "demo")[0]

    store.remove_account("codeforces", "demo")
    profile = store.load_profile()
    assert profile.accounts == []
    assert store.load_submissions("codeforces", "demo") == ([], 0)


def test_merge_deduplicates_and_sorts(tmp_path):
    store = make_store(tmp_path)
    store.merge_submissions("codeforces", "demo", [submission("1", 1000), submission("2", 2000)])
    # 重复 id + 新 id + 乱序
    added = store.merge_submissions(
        "codeforces", "demo",
        [submission("2", 2000), submission("3", 1500), submission("1", 1000)],
    )
    assert added == 1
    items, skipped = store.load_submissions("codeforces", "demo")
    assert skipped == 0
    assert [s.submission_id for s in items] == ["1", "3", "2"]  # 按时间升序
    assert items[1].submitted_at == 1500


def test_load_skips_corrupt_lines(tmp_path):
    store = make_store(tmp_path)
    path = store._submissions_file("codeforces", "demo")
    path.parent.mkdir(parents=True)
    path.write_text(
        submission("1", 1000).model_dump_json() + "\n"
        + "{not valid json}\n"
        + submission("2", 2000).model_dump_json() + "\n",
        encoding="utf-8",
    )
    items, skipped = store.load_submissions("codeforces", "demo")
    assert [s.submission_id for s in items] == ["1", "2"]
    assert skipped == 1


def test_profile_corrupt_falls_back_to_empty(tmp_path):
    store = make_store(tmp_path)
    (tmp_path / "user" / "default").mkdir(parents=True)
    (tmp_path / "user" / "default" / "profile.json").write_text(
        "{broken", encoding="utf-8"
    )
    profile = store.load_profile()
    assert profile.accounts == []


def test_invalid_handle_rejected(tmp_path):
    store = make_store(tmp_path)
    with pytest.raises(BadRequestError):
        store.merge_submissions("codeforces", "a/b", [submission("1", 1000)])
    with pytest.raises(BadRequestError):
        store.load_submissions("codeforces", "../evil")


# ===== 用户组目录管理 =====


def test_profile_roundtrip_with_profile_fields(tmp_path):
    store = make_store(tmp_path)
    profile = store.load_profile()
    profile.id = "我的昵称"
    profile.signature = "菜就多练"
    profile.avatar = "data:image/jpeg;base64,abc"
    store.save_profile(profile)

    loaded = store.load_profile()
    assert loaded.id == "我的昵称"
    assert loaded.signature == "菜就多练"
    assert loaded.avatar == "data:image/jpeg;base64,abc"


def test_create_group_initializes_profile(tmp_path):
    root = tmp_path / "user"
    create_group(root, "第一组")
    groups = list_groups(root)
    assert groups == ["第一组"]
    profile = UserStore(root, "第一组").load_profile()
    # 信息卡 ID 初始为目录名（之后独立编辑）
    assert profile.id == "第一组"
    assert profile.accounts == []


def test_create_group_conflict(tmp_path):
    root = tmp_path / "user"
    create_group(root, "groupA")
    with pytest.raises(ConflictError):
        create_group(root, "groupA")


def test_list_groups_excludes_example_and_hidden(tmp_path):
    root = tmp_path / "user"
    create_group(root, "B组")
    create_group(root, "A组")
    # 模拟 example 样例目录与隐藏目录
    (root / EXAMPLE_GROUP).mkdir(exist_ok=True)
    (root / ".hidden").mkdir()
    assert list_groups(root) == ["A组", "B组"]


def test_rename_group_moves_data(tmp_path):
    root = tmp_path / "user"
    create_group(root, "旧名")
    store = UserStore(root, "旧名")
    store.merge_submissions("codeforces", "demo", [submission("1", 1000)])
    store.save_account(Account(platform="codeforces", handle="demo", last_synced_at=1000))

    rename_group(root, "旧名", "新名")
    assert list_groups(root) == ["新名"]
    # 数据随目录迁移
    moved = UserStore(root, "新名").load_profile()
    assert moved.accounts[0].handle == "demo"
    assert UserStore(root, "新名").load_submissions("codeforces", "demo")[1] == 0
    assert not UserStore(root, "旧名").load_profile().accounts


def test_rename_group_conflict_and_missing(tmp_path):
    root = tmp_path / "user"
    create_group(root, "A")
    create_group(root, "B")
    with pytest.raises(ConflictError):
        rename_group(root, "A", "B")
    with pytest.raises(NotFoundError):
        rename_group(root, "不存在", "C")


def test_delete_group_removes_tree(tmp_path):
    root = tmp_path / "user"
    create_group(root, "A")
    create_group(root, "B")
    delete_group(root, "A")
    assert list_groups(root) == ["B"]
    with pytest.raises(NotFoundError):
        delete_group(root, "A")


def test_chinese_group_name_allowed(tmp_path):
    """中文组名正常放行，路径安全。"""
    root = tmp_path / "user"
    name = create_group(root, "算法训练·秋")
    assert name == "算法训练·秋"
    assert (root / name).is_dir()


# ===== secrets（凭据） =====


def test_secrets_roundtrip(tmp_path):
    """凭据写入/读取往返；不同平台与账号隔离。"""
    store = make_store(tmp_path)
    cred = Credentials(cookies={"_uid": "1", "__client_id": "abc"}, headers={})
    store.save_account_secrets("luogu", "100", cred)
    loaded = store.get_account_credentials("luogu", "100")
    assert loaded is not None
    assert loaded.cookies["__client_id"] == "abc"
    assert store.get_account_credentials("luogu", "200") is None
    assert store.get_account_credentials("codeforces", "demo") is None


def test_secrets_corrupt_falls_back_to_empty(tmp_path):
    """secrets.json 损坏按空凭据集处理（不阻断）。"""
    store = make_store(tmp_path)
    store.save_account_secrets("luogu", "100", Credentials(cookies={"_uid": "1"}))
    (tmp_path / "user" / "default" / "secrets.json").write_text("not json", encoding="utf-8")
    assert store.get_account_credentials("luogu", "100") is None


def test_remove_account_cleans_secrets(tmp_path):
    """解绑/换绑（remove_account）同步清理该账号凭据。"""
    store = make_store(tmp_path)
    store.save_account(Account(platform="luogu", handle="100"))
    store.save_account_secrets("luogu", "100", Credentials(cookies={"_uid": "1"}))
    store.remove_account("luogu", "100")
    assert store.get_account_credentials("luogu", "100") is None


def test_remove_account_secrets_idempotent(tmp_path):
    """删除不存在的凭据为空操作；同平台其他账号凭据不受影响。"""
    store = make_store(tmp_path)
    store.save_account_secrets("luogu", "100", Credentials(cookies={"_uid": "1"}))
    store.remove_account_secrets("luogu", "999")
    assert store.get_account_credentials("luogu", "100") is not None


# ===== update_verdicts（精化受控例外） =====


def test_update_verdicts_in_place(tmp_path):
    """按 submission_id 就地改写 verdict；其他字段与其他记录不受影响。"""
    store = make_store(tmp_path)
    store.merge_submissions(
        "luogu",
        "100",
        [submission("1", 1000, Verdict.UNAC), submission("2", 2000, Verdict.AC)],
    )
    changed = store.update_verdicts("luogu", "100", {"1": Verdict.TLE})
    assert changed == 1
    items, _ = store.load_submissions("luogu", "100")
    by_id = {s.submission_id: s for s in items}
    assert by_id["1"].verdict == Verdict.TLE
    assert by_id["1"].problem_key == "2245A"  # 其余字段不动
    assert by_id["2"].verdict == Verdict.AC  # 其他记录不动


def test_update_verdicts_noop_and_missing(tmp_path):
    """空更新 / 目标不存在 / 值相同均为空操作（返回 0）。"""
    store = make_store(tmp_path)
    store.merge_submissions("luogu", "100", [submission("1", 1000, Verdict.UNAC)])
    assert store.update_verdicts("luogu", "100", {}) == 0
    assert store.update_verdicts("luogu", "100", {"ghost": Verdict.WA}) == 0
    assert store.update_verdicts("luogu", "100", {"1": Verdict.UNAC}) == 0


# ===== 待办清单（review-todo.json） =====


def todo(platform: str = "codeforces", key: str = "2245F", added_at: int = 1000) -> TodoItem:
    return TodoItem(
        platform=platform,
        problem_key=key,
        problem_name="Xor Minimization",
        problem_url="https://codeforces.com/contest/2245/problem/F",
        difficulty=2100,
        tags=["dp", "bitmasks"],
        added_at=added_at,
    )


def test_todo_roundtrip_and_remove(tmp_path):
    """待办写入/读取往返；删除后不再返回，重复删除返回 False。"""
    store = make_store(tmp_path)
    assert store.load_todo() == []

    saved, created = store.add_todo(todo())
    assert created is True
    assert saved.problem_key == "2245F"

    loaded = store.load_todo()
    assert len(loaded) == 1
    assert loaded[0].tags == ["dp", "bitmasks"]

    assert store.remove_todo("codeforces", "2245F") is True
    assert store.load_todo() == []
    assert store.remove_todo("codeforces", "2245F") is False


def test_todo_add_is_idempotent_and_keeps_added_at(tmp_path):
    """重复加入同一题：不产生第二条、addedAt 保留首次、画像字段刷新。"""
    store = make_store(tmp_path)
    store.add_todo(todo(added_at=1000))
    updated = todo(added_at=2000)
    updated.problem_name = "New Name"
    saved, created = store.add_todo(updated)

    assert created is False
    assert saved.added_at == 1000  # 不把题顶到列表最前
    assert saved.problem_name == "New Name"
    assert len(store.load_todo()) == 1


def test_todo_add_keeps_old_tags_when_new_snapshot_empty(tmp_path):
    """离线加入（tags=[]）不该把已有花片清空：空快照保留旧标签。"""
    store = make_store(tmp_path)
    store.add_todo(todo())
    empty = todo()
    empty.tags = []
    saved, _ = store.add_todo(empty)
    assert saved.tags == ["dp", "bitmasks"]


def test_todo_corrupt_file_falls_back_to_empty(tmp_path):
    """review-todo.json 损坏按空清单处理（不阻断待办页）。"""
    store = make_store(tmp_path)
    path = tmp_path / "user" / "default" / "activity" / "review-todo.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("not json", encoding="utf-8")
    assert store.load_todo() == []


def test_todo_skips_broken_entry(tmp_path):
    """单条损坏只跳过该条，其余条目照常返回。"""
    store = make_store(tmp_path)
    path = tmp_path / "user" / "default" / "activity" / "review-todo.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(
            {
                    "items": [
                        {
                            "platform": "codeforces",
                            "problem_key": "2245F",
                            "added_at": 1000,
                        },
                        {"platform": "codeforces"},  # 缺 problem_key / added_at
                    ]
            }
        ),
        encoding="utf-8",
    )
    items = store.load_todo()
    assert [it.problem_key for it in items] == ["2245F"]


def test_todo_isolated_between_groups(tmp_path):
    """待办随用户组目录隔离：另一组看不到。"""
    root = tmp_path / "user"
    UserStore(root, "a").add_todo(todo())
    assert UserStore(root, "b").load_todo() == []


def test_add_todos_writes_once_and_keeps_semantics(tmp_path):
    """批量加入只落盘一次；幂等/刷新画像/保留 added_at 与单条一致。"""
    store = make_store(tmp_path)
    store.add_todo(todo(key="2245F", added_at=1000))

    renamed = todo(key="2245F", added_at=2000)
    renamed.problem_name = "Renamed"
    fresh = todo(key="2245G", added_at=3000)

    out = store.add_todos([renamed, fresh])

    # 返回与入参等长、顺序一致，created 如实反映新建与否
    assert [(it.problem_key, created) for it, created in out] == [
        ("2245F", False),
        ("2245G", True),
    ]
    assert out[0][0].added_at == 1000  # 不把题顶到列表最前
    assert out[0][0].problem_name == "Renamed"
    assert {it.problem_key for it in store.load_todo()} == {"2245F", "2245G"}


def test_add_todos_empty_batch_does_not_touch_file(tmp_path):
    """空批次不落盘：不凭空造出一个空的 review-todo.json。"""
    store = make_store(tmp_path)
    assert store.add_todos([]) == []
    assert not (tmp_path / "user" / "default" / "activity" / "review-todo.json").exists()

