"""content/ 写入操作（可视化增删改）的单元测试与路由测试。"""

import datetime
from pathlib import Path

import pytest
from fastapi.testclient import TestClient

from core.config import Settings
from core.exceptions import (
    BadRequestError,
    ConflictError,
    NotFoundError,
    PayloadTooLargeError,
)
from main import create_app
from modules.template import writer
from modules.template.parser import parse_readme_text
from modules.template.scanner import VISUAL_HISTORY_DIR, find_visual_file, scan_content
from modules.template.schemas import VersionMetaInput, VersionUpsert
from services.template.service import TemplateService, get_template_service


def _payload(**kwargs: object) -> VersionUpsert:
    """构造一个合法的版本请求体，测试按需覆盖字段。"""
    base: dict[str, object] = {
        "name": "v1",
        "ext": "cpp",
        "code": "#include <bits/stdc++.h>\n",
        "meta": VersionMetaInput(),
        "body": "",
    }
    base.update(kwargs)
    return VersionUpsert.model_validate(base)


# ===== 名称与扩展名校验 =====


def test_validate_name_ok() -> None:
    assert writer.validate_name(" 线段树 ", "模板") == "线段树"
    assert writer.validate_name("segtree_lazy", "模板") == "segtree_lazy"


@pytest.mark.parametrize(
    "name",
    ["", "   ", "a/b", "a\\b", "a:b", "a*b", ".hidden", "dir.", "..", "a..b", "~", "CON", "com1"],
)
def test_validate_name_rejects_bad_names(name: str) -> None:
    with pytest.raises(BadRequestError):
        writer.validate_name(name, "模板")


def test_validate_name_too_long() -> None:
    with pytest.raises(BadRequestError):
        writer.validate_name("x" * 101, "模板")


def test_validate_ext() -> None:
    assert writer.validate_ext("CPP") == "cpp"
    assert writer.validate_ext(".py") == "py"
    with pytest.raises(BadRequestError):
        writer.validate_ext("rs")


def test_validate_code_filename() -> None:
    assert writer.validate_code_filename("segtree.cpp", "cpp") == "segtree.cpp"
    with pytest.raises(BadRequestError):
        writer.validate_code_filename("segtree.py", "cpp")  # 后缀与语言不符
    with pytest.raises(BadRequestError):
        writer.validate_code_filename("a/b.cpp", "cpp")


# ===== README 生成与回环 =====


def test_render_readme_roundtrip() -> None:
    """写出的 README 必须能被 parser 原样读回（回环一致）。"""
    meta = VersionMetaInput(
        updated=datetime.date(2026, 8, 5),
        tags=["素数", "数论"],
        source="洛谷 P3383",
        page="https://www.luogu.com.cn/problem/P3383",
        priority=5,
    )
    text = writer.render_readme(meta, "线性筛说明。\n\n第二段。")
    parsed, body = parse_readme_text(text, "test/README.md", [])
    assert parsed.updated == datetime.date(2026, 8, 5)
    assert parsed.tags == ["素数", "数论"]
    assert parsed.source == "洛谷 P3383"
    assert parsed.page == "https://www.luogu.com.cn/problem/P3383"
    assert parsed.priority == 5
    assert body == "线性筛说明。\n\n第二段。"


def test_render_readme_omits_defaults() -> None:
    """缺省字段不写入 front matter；全空时也产出合法的 front matter。"""
    text = writer.render_readme(VersionMetaInput(), "")
    parsed, body = parse_readme_text(text, "test/README.md", [])
    assert parsed.updated is None
    assert parsed.tags == []
    assert parsed.priority == 2
    assert body == ""
    assert "priority" not in text


# ===== 模板（主标签）操作 =====


def test_create_template_dir(content_dir: Path) -> None:
    category, name = writer.create_template_dir(content_dir, "图论", " 最短路 ")
    assert (category, name) == ("图论", "最短路")
    assert (content_dir / "图论" / "最短路").is_dir()
    # 扫描可见为空主标签
    scan = scan_content(content_dir)
    node = next(t for t in scan.templates if t.id == "图论/最短路")
    assert node.versions == []


def test_create_template_dir_conflict(content_dir: Path) -> None:
    with pytest.raises(ConflictError):
        writer.create_template_dir(content_dir, "math", "sieve")


def test_rename_template_dir(content_dir: Path) -> None:
    new_cat, new_name = writer.rename_template_dir(
        content_dir, "math", "sieve", new_category="数学", new_name="线性筛法"
    )
    assert (new_cat, new_name) == ("数学", "线性筛法")
    assert not (content_dir / "math" / "sieve").exists()
    assert (content_dir / "数学" / "线性筛法" / "euler_sieve.cpp").is_file()
    # 原分类 math 已无其他模板，被顺手清理
    assert not (content_dir / "math").exists()


def test_rename_template_dir_conflict(content_dir: Path) -> None:
    with pytest.raises(ConflictError):
        writer.rename_template_dir(content_dir, "math", "sieve", new_name="bare", new_category="misc")
    with pytest.raises(NotFoundError):
        writer.rename_template_dir(content_dir, "math", "not-exist", new_name="x")


def test_delete_template_dir(content_dir: Path) -> None:
    # 空主标签可删
    writer.delete_template_dir(content_dir, "misc", "empty-tpl")
    assert not (content_dir / "misc" / "empty-tpl").exists()
    # 非空模板拒绝删除
    with pytest.raises(ConflictError):
        writer.delete_template_dir(content_dir, "math", "sieve")
    with pytest.raises(NotFoundError):
        writer.delete_template_dir(content_dir, "math", "not-exist")


def test_delete_template_tree(content_dir: Path) -> None:
    writer.delete_template_tree(content_dir, "ds", "dsu")
    assert not (content_dir / "ds" / "dsu").exists()
    with pytest.raises(NotFoundError):
        writer.delete_template_tree(content_dir, "ds", "dsu")


# ===== 版本操作 =====


def test_create_version_dir(content_dir: Path) -> None:
    payload = _payload(
        name="懒标记",
        meta=VersionMetaInput(updated=datetime.date(2026, 8, 5), priority=5),
        body="支持区间修改。",
    )
    slug = writer.create_version_dir(content_dir, "misc", "empty-tpl", payload)
    assert slug == "懒标记"
    version_dir = content_dir / "misc" / "empty-tpl" / "懒标记"
    assert (version_dir / "code.cpp").is_file()
    assert (version_dir / "README.md").is_file()
    # 回环：扫描读回与写入一致
    scan = scan_content(content_dir)
    node = next(t for t in scan.templates if t.id == "misc/empty-tpl")
    assert len(node.versions) == 1
    version = node.versions[0]
    assert version.name == "懒标记"
    assert version.meta.priority == 5
    assert version.meta.updated == datetime.date(2026, 8, 5)
    assert version.body == "支持区间修改。"
    assert "bits/stdc" in version.code


def test_create_version_dir_conflict_and_bad_payload(content_dir: Path) -> None:
    with pytest.raises(ConflictError):
        writer.create_version_dir(content_dir, "ds", "dsu", _payload(name="path-compression"))
    with pytest.raises(BadRequestError):
        writer.create_version_dir(content_dir, "ds", "dsu", _payload(name=None))
    with pytest.raises(BadRequestError):
        writer.create_version_dir(
            content_dir,
            "ds",
            "dsu",
            _payload(meta=VersionMetaInput(page="https://example.com")),  # page 无 source
        )
    with pytest.raises(NotFoundError):
        writer.create_version_dir(content_dir, "ds", "not-exist", _payload())


def test_update_version_dir_rename_and_ext_change(content_dir: Path) -> None:
    # 改名 + 换扩展名 + 全量重写元数据
    payload = _payload(
        name="路径压缩优化",
        ext="py",
        code="def find(x):\n    pass\n",
        meta=VersionMetaInput(tags=["连通性"]),
    )
    new_slug = writer.update_version_dir(
        content_dir, "ds", "dsu", "path-compression", "dsu.cpp", payload
    )
    assert new_slug == "路径压缩优化"
    new_dir = content_dir / "ds" / "dsu" / "路径压缩优化"
    assert new_dir.is_dir()
    assert not (content_dir / "ds" / "dsu" / "path-compression").exists()
    assert (new_dir / "code.py").is_file()
    assert not (new_dir / "dsu.cpp").exists()  # 旧代码文件被清理
    scan = scan_content(content_dir)
    node = next(t for t in scan.templates if t.id == "ds/dsu")
    slugs = [v.slug for v in node.versions]
    assert "路径压缩优化" in slugs


def test_update_version_dir_top_level(content_dir: Path) -> None:
    # 顶层单版本：只改内容，不允许改名
    payload = _payload(name=None, code="// new\n", meta=VersionMetaInput(priority=7))
    new_slug = writer.update_version_dir(
        content_dir, "math", "sieve", "", "euler_sieve.cpp", payload
    )
    assert new_slug == ""
    assert (content_dir / "math" / "sieve" / "code.cpp").is_file()
    assert not (content_dir / "math" / "sieve" / "euler_sieve.cpp").exists()
    with pytest.raises(BadRequestError):
        writer.update_version_dir(
            content_dir, "math", "sieve", "", "code.cpp", _payload(name="新名字")
        )


def test_delete_version_dir(content_dir: Path) -> None:
    # 副标签版本：整目录删除
    writer.delete_version_dir(content_dir, "ds", "dsu", "with-weight", "dsu_weight.cpp")
    assert not (content_dir / "ds" / "dsu" / "with-weight").exists()
    # 顶层单版本：删代码、README、visual.html 与 .visual-history/，目录留空成为空主标签
    (content_dir / "math" / "sieve" / "VISUAL.HTML").write_text("<p></p>", encoding="utf-8")
    hist = content_dir / "math" / "sieve" / VISUAL_HISTORY_DIR
    hist.mkdir()
    (hist / "20261020-040112Z.html").write_text("<p>old</p>", encoding="utf-8")
    writer.delete_version_dir(content_dir, "math", "sieve", "", "euler_sieve.cpp")
    template_dir = content_dir / "math" / "sieve"
    assert template_dir.is_dir()
    assert list(template_dir.iterdir()) == []
    scan = scan_content(content_dir)
    node = next(t for t in scan.templates if t.id == "math/sieve")
    assert node.versions == []


def test_write_visual_html_top_level_overwrite_and_reject(content_dir: Path) -> None:
    html = "<!doctype html><html><body>ok</body></html>"
    writer.write_visual_html(content_dir, "math", "sieve", "~", html, 2 * 1024 * 1024)
    path = content_dir / "math" / "sieve" / "visual.html"
    assert path.read_text(encoding="utf-8") == html

    writer.write_visual_html(content_dir, "字符串", "哈希", "~", html, 2 * 1024 * 1024)
    assert (content_dir / "字符串" / "哈希" / "visual.html").read_text(encoding="utf-8") == html

    (content_dir / "math" / "sieve" / "visual.html").unlink()
    (content_dir / "math" / "sieve" / "VISUAL.HTML").write_text("old", encoding="utf-8")
    writer.write_visual_html(content_dir, "math", "sieve", "~", html, 2 * 1024 * 1024)
    found = find_visual_file(content_dir / "math" / "sieve")
    assert found is not None
    assert found.read_text(encoding="utf-8") == html
    html_files = [
        p for p in (content_dir / "math" / "sieve").iterdir() if p.name.lower() == "visual.html"
    ]
    assert len(html_files) == 1

    with pytest.raises(BadRequestError):
        writer.write_visual_html(content_dir, "math", "sieve", "~", "   ", 1000)
    with pytest.raises(BadRequestError):
        writer.write_visual_html(content_dir, "math", "sieve", "~", "int main(){}", 1000)
    with pytest.raises(NotFoundError):
        writer.write_visual_html(content_dir, "math", "sieve", "missing", html, 1000)
    with pytest.raises(PayloadTooLargeError):
        writer.write_visual_html(content_dir, "math", "sieve", "~", html, 8)
    with pytest.raises(BadRequestError):
        writer.write_visual_html(content_dir, "..", "sieve", "~", html, 1000)


def test_visual_history_archive_restore_clear(content_dir: Path) -> None:
    html_a = "<!doctype html><html><body>A</body></html>"
    html_b = "<!doctype html><html><body>B</body></html>"
    writer.write_visual_html(content_dir, "math", "sieve", "~", html_a, 2 * 1024 * 1024)
    writer.write_visual_html(content_dir, "math", "sieve", "~", html_a, 2 * 1024 * 1024)
    assert writer.list_visual_history(content_dir, "math", "sieve", "~") == []

    writer.write_visual_html(content_dir, "math", "sieve", "~", html_b, 2 * 1024 * 1024)
    items = writer.list_visual_history(content_dir, "math", "sieve", "~")
    assert len(items) == 1
    snap_id = str(items[0]["id"])
    assert snap_id.endswith("Z") or "-2" in snap_id
    assert (content_dir / "math" / "sieve" / "visual.html").read_text(
        encoding="utf-8"
    ) == html_b
    got = writer.read_visual_history_bytes(
        content_dir, "math", "sieve", "~", snap_id, 2 * 1024 * 1024
    )
    assert got.decode("utf-8") == html_a

    writer.write_visual_html(
        content_dir, "字符串", "哈希", "~", html_a, 2 * 1024 * 1024
    )
    writer.write_visual_html(
        content_dir, "字符串", "哈希", "~", html_b, 2 * 1024 * 1024
    )
    zh_items = writer.list_visual_history(content_dir, "字符串", "哈希", "~")
    assert len(zh_items) == 1

    writer.write_visual_html(
        content_dir, "ds", "dsu", "path-compression", html_a, 2 * 1024 * 1024
    )
    writer.write_visual_html(
        content_dir, "ds", "dsu", "path-compression", html_b, 2 * 1024 * 1024
    )
    nested = writer.list_visual_history(content_dir, "ds", "dsu", "path-compression")
    assert len(nested) == 1

    scan = scan_content(content_dir)
    sieve = next(t for t in scan.templates if t.id == "math/sieve")
    assert len(sieve.versions) == 1

    writer.restore_visual_history(content_dir, "math", "sieve", "~", snap_id, 2 * 1024 * 1024)
    assert (content_dir / "math" / "sieve" / "visual.html").read_text(
        encoding="utf-8"
    ) == html_a
    after_restore = writer.list_visual_history(content_dir, "math", "sieve", "~")
    assert len(after_restore) == 2

    writer.delete_visual_history_item(content_dir, "math", "sieve", "~", snap_id)
    assert len(writer.list_visual_history(content_dir, "math", "sieve", "~")) == 1
    writer.clear_visual_history(content_dir, "math", "sieve", "~")
    assert writer.list_visual_history(content_dir, "math", "sieve", "~") == []
    assert (content_dir / "math" / "sieve" / "visual.html").read_text(
        encoding="utf-8"
    ) == html_a

    with pytest.raises(BadRequestError):
        writer.read_visual_history_bytes(content_dir, "math", "sieve", "~", "..", 1000)
    with pytest.raises(BadRequestError):
        writer.read_visual_history_bytes(content_dir, "math", "sieve", "~", "nope", 1000)
    with pytest.raises(NotFoundError):
        writer.delete_visual_history_item(
            content_dir, "math", "sieve", "~", "20261020-000000Z"
        )


# ===== 路由测试 =====


@pytest.fixture
def service(tmp_path: Path, content_dir: Path) -> TemplateService:
    settings = Settings(content_dir=content_dir, data_dir=tmp_path / "data")
    svc = TemplateService(settings)
    svc.rebuild()
    return svc


@pytest.fixture
def client(service: TemplateService) -> TestClient:
    app = create_app()
    app.dependency_overrides[get_template_service] = lambda: service
    return TestClient(app)


def test_api_create_and_delete_template(client: TestClient) -> None:
    resp = client.post("/api/templates", json={"category": "图论", "name": "最短路"})
    assert resp.status_code == 201
    detail = resp.json()
    assert detail["id"] == "图论/最短路"
    assert detail["variant_count"] == 0
    # 冲突 → 409
    resp = client.post("/api/templates", json={"category": "图论", "name": "最短路"})
    assert resp.status_code == 409
    assert resp.json()["error"]["code"] == "conflict"
    # 删除
    resp = client.delete("/api/templates/图论/最短路")
    assert resp.status_code == 204
    resp = client.get("/api/templates/图论/最短路")
    assert resp.status_code == 404


def test_api_delete_non_empty_template_rejected(client: TestClient) -> None:
    resp = client.delete("/api/templates/math/sieve")
    assert resp.status_code == 409


def test_api_batch_delete_templates(client: TestClient) -> None:
    resp = client.post(
        "/api/templates/batch-delete",
        json={"ids": ["math/sieve", "ds/dsu", "math/sieve"]},
    )
    assert resp.status_code == 204
    assert client.get("/api/templates/math/sieve").status_code == 404
    assert client.get("/api/templates/ds/dsu").status_code == 404
    # 未点名的模板仍在
    assert client.get("/api/templates/misc/empty-tpl").status_code == 200


def test_api_batch_delete_rejects_invalid_or_missing(client: TestClient) -> None:
    resp = client.post("/api/templates/batch-delete", json={"ids": []})
    assert resp.status_code == 400
    resp = client.post("/api/templates/batch-delete", json={"ids": ["no-slash"]})
    assert resp.status_code == 400
    assert resp.json()["error"]["code"] == "bad_request"
    resp = client.post(
        "/api/templates/batch-delete",
        json={"ids": ["math/sieve", "math/not-exist"]},
    )
    assert resp.status_code == 404
    # 整批拒绝：存在的目标也不应被删
    assert client.get("/api/templates/math/sieve").status_code == 200


def test_api_version_crud(client: TestClient) -> None:
    # 新建版本
    resp = client.post(
        "/api/templates/misc/empty-tpl/versions",
        json={
            "name": "标准实现",
            "ext": "cpp",
            "code": "int main() {}\n",
            "meta": {"priority": 4, "updated": "2026-08-05"},
            "body": "说明。",
        },
    )
    assert resp.status_code == 201
    detail = resp.json()
    assert detail["variant_count"] == 1
    assert detail["variants"][0]["name"] == "标准实现"
    assert detail["variants"][0]["priority"] == 4
    # 更新版本（改优先级）
    resp = client.put(
        "/api/templates/misc/empty-tpl/versions/标准实现",
        json={
            "name": "标准实现",
            "ext": "cpp",
            "code": "int main() { return 0; }\n",
            "meta": {"priority": 6},
            "body": "新说明。",
        },
    )
    assert resp.status_code == 200
    assert resp.json()["variants"][0]["priority"] == 6
    # 删除版本 → 回到空主标签
    resp = client.delete("/api/templates/misc/empty-tpl/versions/标准实现")
    assert resp.status_code == 204
    resp = client.get("/api/templates/misc/empty-tpl")
    assert resp.json()["variant_count"] == 0


def test_api_top_level_version_via_tilde(client: TestClient) -> None:
    """顶层单版本用保留字 ~ 寻址。"""
    resp = client.put(
        "/api/templates/math/sieve/versions/~",
        json={"ext": "cpp", "file": "euler_sieve.cpp", "code": "// v2\n", "meta": {}},
    )
    assert resp.status_code == 200
    assert "v2" in resp.json()["variants"][0]["code"]
    resp = client.delete("/api/templates/math/sieve/versions/~")
    assert resp.status_code == 204
    assert client.get("/api/templates/math/sieve").json()["variant_count"] == 0


def test_api_rename_template(client: TestClient) -> None:
    resp = client.put(
        "/api/templates/ds/dsu", json={"new_category": "数据结构", "new_name": "并查集"}
    )
    assert resp.status_code == 200
    assert resp.json()["id"] == "数据结构/并查集"
    assert client.get("/api/templates/ds/dsu").status_code == 404


def test_api_validation_error_structured(client: TestClient) -> None:
    """请求体缺字段时返回结构化 400 而非 500。"""
    resp = client.post("/api/templates", json={"category": "x"})
    assert resp.status_code == 400
    assert resp.json()["error"]["code"] == "bad_request"
