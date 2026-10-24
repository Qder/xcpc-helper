"""模板服务的单元测试（含 FastAPI 路由冒烟测试）。"""

import datetime
from pathlib import Path

import pytest
from fastapi.testclient import TestClient

from core.config import Settings
from core.exceptions import BadRequestError
from main import create_app
from modules.template.schemas import VisualUpsert
from services.template.service import TemplateService, get_template_service


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


def test_list_sorted_by_priority(service: TemplateService) -> None:
    items = service.list_templates(sort="priority")
    priorities = [t.priority for t in items]
    assert priorities == sorted(priorities, reverse=True)
    assert items[0].name == "sieve"


def test_list_sorted_by_difficulty(service: TemplateService) -> None:
    items = service.list_templates()
    difficulties = [t.difficulty for t in items]
    assert difficulties == sorted(difficulties, reverse=True)
    # dsu max=6，sieve=7；默认难度降序，难的在前
    dsu = next(t for t in items if t.id == "ds/dsu")
    sieve = next(t for t in items if t.id == "math/sieve")
    assert items.index(sieve) < items.index(dsu)
    assert dsu.difficulty == 6
    assert sieve.difficulty == 7


def test_list_filter_by_category_and_keyword(service: TemplateService) -> None:
    assert [t.id for t in service.list_templates(category="ds")] == ["ds/dsu"]
    hits = service.list_templates(keyword="并查集")
    assert {t.id for t in hits} == {"ds/dsu"}


def test_list_search_sorted_by_relevance(tmp_path: Path) -> None:
    """搜索时相关度恒为第一排序键：同名模板压过高优先级的间接命中。"""
    root = tmp_path / "content"

    def _write(path: Path, text: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    _write(root / "ds" / "线段树" / "seg.cpp", "int seg[4];\n")
    _write(root / "ds" / "线段树" / "README.md", "---\npriority: 1\n---\n")
    _write(root / "ds" / "lct" / "lct.cpp", "struct LCT {};\n")
    _write(
        root / "ds" / "lct" / "README.md",
        "---\npriority: 9\n---\n\n内部用线段树维护信息。\n",
    )

    settings = Settings(content_dir=root, data_dir=tmp_path / "data")
    svc = TemplateService(settings)
    svc.rebuild()

    # 不搜索时仍按优先级：lct 在前
    assert [t.id for t in svc.list_templates()] == ["ds/lct", "ds/线段树"]
    # 搜索时按相关度：名称精确命中的“线段树”在前
    hits = svc.list_templates(keyword="线段树")
    assert [t.id for t in hits] == ["ds/线段树", "ds/lct"]


def test_list_filter_by_tags(service: TemplateService) -> None:
    hits = service.list_templates(tags=["连通性"])
    assert {t.id for t in hits} == {"ds/dsu"}


def test_detail_with_variants(service: TemplateService) -> None:
    detail = service.get_detail("ds/dsu")
    assert detail.variant_count == 2
    assert [v.id for v in detail.variants] == ["ds/dsu/path-compression", "ds/dsu/with-weight"]
    assert "带权版" in detail.variants[1].body


def test_detail_variants_carry_meta(service: TemplateService) -> None:
    """详情接口的版本携带各自元信息，前端切换版本时随动展示。"""
    detail = service.get_detail("ds/dsu")
    first, second = detail.variants
    assert first.priority == 4
    assert first.difficulty == 6
    assert first.updated == datetime.date(2026, 7, 1)
    assert second.priority == 3
    assert second.difficulty == 3
    assert second.updated == datetime.date(2026, 7, 10)
    assert second.tags == ["连通性"]
    assert first.examples == []
    assert second.examples == []


def test_detail_not_found(service: TemplateService) -> None:
    from core.exceptions import NotFoundError

    with pytest.raises(NotFoundError):
        service.get_detail("not/exist")


def test_detail_empty_template(service: TemplateService) -> None:
    """空主标签：详情返回空版本列表，主版本字段为 None。"""
    detail = service.get_detail("misc/empty-tpl")
    assert detail.variant_count == 0
    assert detail.variants == []
    assert detail.lang is None
    assert detail.file is None
    assert detail.priority == 2
    assert detail.difficulty == 5


def test_categories(service: TemplateService) -> None:
    cats = {c.id: c.count for c in service.list_categories()}
    assert cats["字符串"] == 1
    assert cats["ds"] == 1


def test_list_and_detail_has_visual(tmp_path: Path, content_dir: Path) -> None:
    (content_dir / "math" / "sieve" / "visual.html").write_text("<p></p>", encoding="utf-8")
    (content_dir / "ds" / "dsu" / "path-compression" / "visual.html").write_text(
        "<p></p>", encoding="utf-8"
    )
    settings = Settings(content_dir=content_dir, data_dir=tmp_path / "data")
    svc = TemplateService(settings)
    svc.rebuild()
    only = svc.list_templates(has_visual=True)
    assert {t.id for t in only} == {"math/sieve", "ds/dsu"}
    assert all(t.has_visual for t in only)
    all_items = svc.list_templates()
    assert any(not t.has_visual for t in all_items)
    detail = svc.get_detail("ds/dsu")
    assert detail.has_visual is True
    assert detail.variants[0].has_visual is True
    assert detail.variants[1].has_visual is False


def test_api_templates(client: TestClient) -> None:
    resp = client.get("/api/templates")
    assert resp.status_code == 200
    data = resp.json()
    assert any(t["id"] == "math/sieve" for t in data)


def test_api_templates_query(client: TestClient) -> None:
    resp = client.get("/api/templates", params={"keyword": "素数", "category": "math"})
    assert resp.status_code == 200
    assert [t["id"] for t in resp.json()] == ["math/sieve"]


def test_api_has_visual_filter(tmp_path: Path, content_dir: Path) -> None:
    (content_dir / "math" / "sieve" / "visual.html").write_text("<p></p>", encoding="utf-8")
    settings = Settings(content_dir=content_dir, data_dir=tmp_path / "data")
    svc = TemplateService(settings)
    svc.rebuild()
    app = create_app()
    app.dependency_overrides[get_template_service] = lambda: svc
    client = TestClient(app)
    resp = client.get("/api/templates", params={"has_visual": True})
    assert resp.status_code == 200
    ids = {t["id"] for t in resp.json()}
    assert ids == {"math/sieve"}
    assert all(t["has_visual"] is True for t in resp.json())
    detail = client.get("/api/templates/math/sieve").json()
    assert detail["has_visual"] is True
    assert detail["variants"][0]["has_visual"] is True
    empty = client.get("/api/templates/misc/empty-tpl").json()
    assert empty["has_visual"] is False


def test_api_detail_and_404(client: TestClient) -> None:
    resp = client.get("/api/templates/ds/dsu")
    assert resp.status_code == 200
    assert resp.json()["variant_count"] == 2
    resp = client.get("/api/templates/not/exist")
    assert resp.status_code == 404
    assert resp.json()["error"]["code"] == "not_found"


def test_api_categories_and_diagnostics(client: TestClient) -> None:
    resp = client.get("/api/categories")
    assert resp.status_code == 200
    assert any(c["id"] == "math" for c in resp.json())
    resp = client.get("/api/diagnostics")
    assert resp.status_code == 200
    assert len(resp.json()["items"]) > 0  # 测试样本含坏格式文件


def test_api_reload(client: TestClient) -> None:
    resp = client.post("/api/templates/reload")
    assert resp.status_code == 200
    assert resp.json()["templates"] > 0


def _visual_client(tmp_path: Path, content_dir: Path) -> TestClient:
    settings = Settings(content_dir=content_dir, data_dir=tmp_path / "data")
    svc = TemplateService(settings)
    svc.rebuild()
    app = create_app()
    app.dependency_overrides[get_template_service] = lambda: svc
    return TestClient(app)


def test_api_visual_headers_tilde_and_chinese(tmp_path: Path, content_dir: Path) -> None:
    html = "<html><body>sieve-demo</body></html>"
    (content_dir / "math" / "sieve" / "visual.html").write_text(html, encoding="utf-8")
    (content_dir / "ds" / "dsu" / "path-compression" / "visual.html").write_text(
        "dsu", encoding="utf-8"
    )
    (content_dir / "字符串" / "哈希" / "visual.html").write_text("哈希演示", encoding="utf-8")
    client = _visual_client(tmp_path, content_dir)

    resp = client.get("/api/templates/math/sieve/versions/~/visual")
    assert resp.status_code == 200
    assert resp.headers["content-type"].startswith("text/html")
    assert "charset=utf-8" in resp.headers["content-type"]
    assert resp.headers["x-content-type-options"] == "nosniff"
    csp = resp.headers["content-security-policy"]
    assert "sandbox" in csp
    assert "connect-src 'none'" in csp
    assert resp.content.decode("utf-8") == html

    resp = client.get("/api/templates/ds/dsu/versions/path-compression/visual")
    assert resp.status_code == 200
    assert resp.content == b"dsu"

    resp = client.get("/api/templates/字符串/哈希/versions/~/visual")
    assert resp.status_code == 200
    assert resp.content.decode("utf-8") == "哈希演示"


def test_api_visual_404_413_and_bad_name(tmp_path: Path, content_dir: Path) -> None:
    client = _visual_client(tmp_path, content_dir)
    resp = client.get("/api/templates/math/sieve/versions/~/visual")
    assert resp.status_code == 404
    resp = client.get("/api/templates/ds/dsu/versions/missing/visual")
    assert resp.status_code == 404

    (content_dir / "math" / "sieve" / "visual.html").write_bytes(b"x" * (2 * 1024 * 1024 + 1))
    resp = client.get("/api/templates/math/sieve/versions/~/visual")
    assert resp.status_code == 413
    assert resp.json()["error"]["code"] == "payload_too_large"

    settings = Settings(content_dir=content_dir, data_dir=tmp_path / "data2")
    svc = TemplateService(settings)
    with pytest.raises(BadRequestError):
        svc.read_visual("..", "sieve", "~")
    with pytest.raises(BadRequestError):
        svc.read_visual("math", "sieve", "..")
    resp = client.get("/api/templates/CON/sieve/versions/~/visual")
    assert resp.status_code == 400


def test_api_put_visual_then_get(tmp_path: Path, content_dir: Path) -> None:
    client = _visual_client(tmp_path, content_dir)
    html = "<!doctype html><html lang='zh'><body>导入</body></html>"
    resp = client.put(
        "/api/templates/math/sieve/versions/~/visual",
        json={"html": html},
    )
    assert resp.status_code == 204
    got = client.get("/api/templates/math/sieve/versions/~/visual")
    assert got.status_code == 200
    assert got.content.decode("utf-8") == html
    listed = client.get("/api/templates", params={"has_visual": True})
    assert any(t["id"] == "math/sieve" for t in listed.json())

    resp = client.put(
        "/api/templates/ds/dsu/versions/path-compression/visual",
        json={"html": html},
    )
    assert resp.status_code == 204
    resp = client.put(
        "/api/templates/字符串/哈希/versions/~/visual",
        json={"html": html},
    )
    assert resp.status_code == 204

    resp = client.put("/api/templates/math/sieve/versions/~/visual", json={"html": "   "})
    assert resp.status_code == 400
    resp = client.put(
        "/api/templates/math/sieve/versions/~/visual", json={"html": "not html"}
    )
    assert resp.status_code == 400
    resp = client.put(
        "/api/templates/ds/dsu/versions/missing/visual", json={"html": html}
    )
    assert resp.status_code == 404
    huge = "<html>" + ("x" * (2 * 1024 * 1024)) + "</html>"
    resp = client.put("/api/templates/math/sieve/versions/~/visual", json={"html": huge})
    assert resp.status_code == 413

    settings = Settings(content_dir=content_dir, data_dir=tmp_path / "data-put")
    svc = TemplateService(settings)
    with pytest.raises(BadRequestError):
        svc.write_visual("..", "sieve", "~", VisualUpsert(html=html))
    with pytest.raises(BadRequestError):
        svc.write_visual("math", "sieve", "..", VisualUpsert(html=html))


def test_api_visual_history_restore_and_clear(tmp_path: Path, content_dir: Path) -> None:
    client = _visual_client(tmp_path, content_dir)
    html_a = "<!doctype html><html><body>A</body></html>"
    html_b = "<!doctype html><html><body>B</body></html>"
    base = "/api/templates/math/sieve/versions/~/visual"
    assert client.put(base, json={"html": html_a}).status_code == 204
    assert client.put(base, json={"html": html_b}).status_code == 204

    listed = client.get(f"{base}/history")
    assert listed.status_code == 200
    items = listed.json()["items"]
    assert len(items) == 1
    snap_id = items[0]["id"]
    assert items[0]["bytes"] == len(html_a.encode("utf-8"))
    assert items[0]["saved_at"].endswith("Z")

    got = client.get(f"{base}/history/{snap_id}")
    assert got.status_code == 200
    assert got.headers["x-content-type-options"] == "nosniff"
    assert "sandbox" in got.headers["content-security-policy"]
    assert got.content.decode("utf-8") == html_a

    zh_base = "/api/templates/字符串/哈希/versions/~/visual"
    assert client.put(zh_base, json={"html": html_a}).status_code == 204
    assert client.put(zh_base, json={"html": html_b}).status_code == 204
    assert client.get(f"{zh_base}/history").json()["items"]

    nested = "/api/templates/ds/dsu/versions/path-compression/visual"
    assert client.put(nested, json={"html": html_a}).status_code == 204
    assert client.put(nested, json={"html": html_b}).status_code == 204

    assert client.post(f"{base}/history/{snap_id}/restore").status_code == 204
    assert client.get(base).content.decode("utf-8") == html_a

    assert client.delete(f"{base}/history/{snap_id}").status_code == 204
    assert client.delete(f"{base}/history/{snap_id}").status_code == 404
    assert client.delete(f"{base}/history").status_code == 204
    assert client.get(f"{base}/history").json()["items"] == []
    assert client.get(base).content.decode("utf-8") == html_a

    assert client.get(f"{base}/history/nope").status_code == 400
    assert client.get(f"{base}/history/20261020-000000Z").status_code == 404

    settings = Settings(content_dir=content_dir, data_dir=tmp_path / "data-hist")
    svc = TemplateService(settings)
    with pytest.raises(BadRequestError):
        svc.read_visual_history("math", "sieve", "~", "..")
    with pytest.raises(BadRequestError):
        svc.read_visual_history("..", "sieve", "~", "20261020-000000Z")
