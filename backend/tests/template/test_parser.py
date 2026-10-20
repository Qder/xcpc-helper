"""README 解析器的单元测试。"""

from modules.template.models import Diagnostic
from modules.template.parser import parse_readme_text


def _parse(text: str) -> tuple:
    diags: list[Diagnostic] = []
    meta, body = parse_readme_text(text, "test/README.md", diags)
    return meta, body, diags


def test_parse_full_front_matter() -> None:
    text = (
        "---\n"
        "updated: 2026-07-29\n"
        "tags: ['素数', '积性函数']\n"
        "source: '洛谷 P3383'\n"
        "page: 'https://www.luogu.com.cn/problem/P3383'\n"
        "priority: 5\n"
        "difficulty: 7\n"
        "---\n"
        "\n"
        "正文说明。\n"
    )
    meta, body, diags = _parse(text)
    assert meta.updated is not None and meta.updated.isoformat() == "2026-07-29"
    assert meta.tags == ["素数", "积性函数"]
    assert meta.source == "洛谷 P3383"
    assert meta.page == "https://www.luogu.com.cn/problem/P3383"
    assert meta.priority == 5
    assert meta.difficulty == 7
    assert body == "正文说明。"
    assert diags == []


def test_parse_defaults_and_optional_fields() -> None:
    meta, body, diags = _parse("---\n{}\n---\n")
    assert meta.priority == 2  # 默认优先级
    assert meta.difficulty == 5  # 默认难度
    assert meta.tags == []
    assert meta.updated is None
    assert meta.examples == []
    assert body == ""
    assert diags == []


def test_unknown_fields_tolerated() -> None:
    # 规范之外的字段被静默保留，不产生诊断（向前兼容）
    meta, _, diags = _parse("---\ntags: [a]\nfuture_field: 'x'\n---\n")
    assert meta.tags == ["a"]
    assert diags == []


def test_parse_examples() -> None:
    text = (
        "---\n"
        "examples:\n"
        "  - label: CF 26A\n"
        "    url: https://codeforces.com/problemset/problem/26/A\n"
        "  - url: https://www.luogu.com.cn/problem/P3383\n"
        "---\n"
    )
    meta, _, diags = _parse(text)
    assert [(e.label, e.url) for e in meta.examples] == [
        ("CF 26A", "https://codeforces.com/problemset/problem/26/A"),
        ("", "https://www.luogu.com.cn/problem/P3383"),
    ]
    assert diags == []


def test_examples_invalid_items_dropped() -> None:
    text = (
        "---\n"
        "examples:\n"
        "  - label: ok\n"
        "    url: https://example.com/a\n"
        "  - label: no-url\n"
        "  - url: javascript:alert(1)\n"
        "  - just-a-string\n"
        "---\n"
    )
    meta, _, diags = _parse(text)
    assert [(e.label, e.url) for e in meta.examples] == [
        ("ok", "https://example.com/a"),
    ]
    assert any("examples" in d.message for d in diags)


def test_examples_over_limit_truncated() -> None:
    items = "\n".join(
        f"  - url: https://example.com/{i}" for i in range(14)
    )
    meta, _, diags = _parse(f"---\nexamples:\n{items}\n---\n")
    assert len(meta.examples) == 12
    assert any("12" in d.message for d in diags)


def test_page_without_source_warns() -> None:
    _, _, diags = _parse("---\npage: 'https://a.b'\n---\n")
    assert any(d.level == "warning" and "source" in d.message for d in diags)


def test_bad_yaml_falls_back_gracefully() -> None:
    meta, body, diags = _parse("---\ntags: [unclosed\n---\n正文保留。\n")
    assert meta.priority == 2  # 兜底为默认元数据
    assert body == "正文保留。"
    assert any(d.level == "error" and "YAML" in d.message for d in diags)


def test_no_front_matter_treated_as_plain_text() -> None:
    meta, body, diags = _parse("这只是一段说明。\n")
    assert meta.priority == 2
    assert body == "这只是一段说明。"
    assert any("front matter" in d.message for d in diags)


def test_tags_accepts_single_string() -> None:
    meta, _, _ = _parse("---\ntags: '连通性'\n---\n")
    assert meta.tags == ["连通性"]


def test_bad_updated_date_warns_and_ignored() -> None:
    meta, _, diags = _parse("---\nupdated: 'not-a-date'\n---\n")
    assert meta.updated is None
    assert any(d.level == "warning" and "updated" in d.message for d in diags)


def test_bom_and_crlf_tolerated() -> None:
    meta, body, _ = _parse("\ufeff---\r\npriority: 4\r\n---\r\n\r\nbody\r\n")
    assert meta.priority == 4
    assert body == "body"


def test_difficulty_accepts_string_number() -> None:
    meta, _, _ = _parse("---\ndifficulty: '3'\n---\n")
    assert meta.difficulty == 3
