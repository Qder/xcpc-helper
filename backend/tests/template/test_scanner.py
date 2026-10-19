"""content/ 目录扫描器的单元测试。"""

from pathlib import Path

from modules.template.scanner import scan_content


def test_scan_all_forms(content_dir: Path) -> None:
    result = scan_content(content_dir)
    ids = {t.id for t in result.templates}
    # broken-no-code 有内容但无版本被跳过；bare 为极简但合法的元数据样本；
    # empty-tpl 是完全空的目录，作为空主标签正常载入
    assert ids == {
        "math/sieve",
        "ds/dsu",
        "graph/tarjan",
        "字符串/哈希",
        "misc/bare",
        "misc/empty-tpl",
    }


def test_empty_template_dir(content_dir: Path) -> None:
    """完全空的模板目录：载入为无版本的空主标签，不产生诊断。"""
    result = scan_content(content_dir)
    empty = next(t for t in result.templates if t.id == "misc/empty-tpl")
    assert empty.versions == []
    assert not any(d.path == "misc/empty-tpl" for d in result.diagnostics)


def test_single_version_template(content_dir: Path) -> None:
    result = scan_content(content_dir)
    sieve = next(t for t in result.templates if t.id == "math/sieve")
    assert len(sieve.versions) == 1
    version = sieve.versions[0]
    assert version.slug == ""  # 单版本无副标签
    assert version.name == "sieve"  # 显示名取自目录名
    assert version.lang == "cpp"
    assert version.meta.priority == 5
    assert "最小质因子" in version.body


def test_multi_version_template(content_dir: Path) -> None:
    result = scan_content(content_dir)
    dsu = next(t for t in result.templates if t.id == "ds/dsu")
    assert [v.slug for v in dsu.versions] == ["path-compression", "with-weight"]
    assert all(v.lang == "cpp" for v in dsu.versions)


def test_single_subdir_collapsed_to_single_version(content_dir: Path) -> None:
    result = scan_content(content_dir)
    tarjan = next(t for t in result.templates if t.id == "graph/tarjan")
    assert len(tarjan.versions) == 1


def test_chinese_paths(content_dir: Path) -> None:
    result = scan_content(content_dir)
    hashed = next(t for t in result.templates if t.id == "字符串/哈希")
    assert hashed.category == "字符串"
    assert hashed.versions[0].name == "哈希"
    # page 填写但 source 缺失 → 产生告警
    assert any("source" in d.message for d in result.diagnostics)


def test_diagnostics_collected(content_dir: Path) -> None:
    result = scan_content(content_dir)
    messages = [d.message for d in result.diagnostics]
    assert any("未找到任何可用版本" in m for m in messages)  # broken-no-code
    assert any("updated" in m for m in messages)  # tarjan 的坏日期


def test_has_visual_absent_by_default(content_dir: Path) -> None:
    result = scan_content(content_dir)
    assert all(not v.has_visual for t in result.templates for v in t.versions)


def test_has_visual_top_level_subdir_chinese_and_casefold(content_dir: Path) -> None:
    """visual.html 大小写不敏感；顶层 / 子目录 / 中文路径均识别；不产生诊断。"""
    before = {d.message for d in scan_content(content_dir).diagnostics}
    (content_dir / "math" / "sieve" / "visual.html").write_text("<html></html>", encoding="utf-8")
    (content_dir / "ds" / "dsu" / "path-compression" / "VISUAL.HTML").write_text(
        "<html></html>", encoding="utf-8"
    )
    (content_dir / "字符串" / "哈希" / "Visual.HTML").write_text("<html></html>", encoding="utf-8")
    result = scan_content(content_dir)
    sieve = next(t for t in result.templates if t.id == "math/sieve")
    assert sieve.versions[0].has_visual
    dsu = next(t for t in result.templates if t.id == "ds/dsu")
    by_slug = {v.slug: v.has_visual for v in dsu.versions}
    assert by_slug["path-compression"] is True
    assert by_slug["with-weight"] is False
    hashed = next(t for t in result.templates if t.id == "字符串/哈希")
    assert hashed.versions[0].has_visual
    after = {d.message for d in result.diagnostics}
    assert after == before
    assert not any("html" in d.message.lower() for d in result.diagnostics)


def test_missing_content_dir(tmp_path: Path) -> None:
    result = scan_content(tmp_path / "not-exist")
    assert result.templates == []
    assert any(d.level == "error" for d in result.diagnostics)
