#include <gtest/gtest.h>

#include "tapiru/text/constexpr_markup.h"

using namespace tapiru;

// ── markup_style: compile-time tag resolution ───────────────────────────

TEST(ConstexprMarkupTest, SingleAttribute) {
    constexpr auto s = markup_style("bold");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg.is_default());
    static_assert(s.bg.is_default());
    EXPECT_TRUE(has_attr(s.attrs, attr::bold));
}

TEST(ConstexprMarkupTest, SingleColor) {
    constexpr auto s = markup_style("red");
    static_assert(s.fg == colors::red);
    static_assert(s.bg.is_default());
    static_assert(s.attrs == attr::none);
    EXPECT_EQ(s.fg, colors::red);
}

TEST(ConstexprMarkupTest, CompoundTag) {
    constexpr auto s = markup_style("bold red on_blue");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::red);
    static_assert(s.bg == colors::blue);
    EXPECT_TRUE(has_attr(s.attrs, attr::bold));
    EXPECT_EQ(s.fg, colors::red);
    EXPECT_EQ(s.bg, colors::blue);
}

TEST(ConstexprMarkupTest, AllAttributes) {
    constexpr auto s = markup_style("bold dim italic underline blink reverse hidden strike");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(has_attr(s.attrs, attr::dim));
    static_assert(has_attr(s.attrs, attr::italic));
    static_assert(has_attr(s.attrs, attr::underline));
    static_assert(has_attr(s.attrs, attr::blink));
    static_assert(has_attr(s.attrs, attr::reverse));
    static_assert(has_attr(s.attrs, attr::hidden));
    static_assert(has_attr(s.attrs, attr::strike));
    SUCCEED();
}

TEST(ConstexprMarkupTest, BrightColors) {
    constexpr auto s = markup_style("bright_red on_bright_cyan");
    static_assert(s.fg == colors::bright_red);
    static_assert(s.bg == colors::bright_cyan);
    SUCCEED();
}

TEST(ConstexprMarkupTest, HexColor) {
    constexpr auto s = markup_style("#FF8000");
    static_assert(s.fg == color::from_rgb(0xFF, 0x80, 0x00));
    SUCCEED();
}

TEST(ConstexprMarkupTest, HexBgColor) {
    constexpr auto s = markup_style("on_#00FF80");
    static_assert(s.bg == color::from_rgb(0x00, 0xFF, 0x80));
    SUCCEED();
}

TEST(ConstexprMarkupTest, DefaultStyle) {
    constexpr auto s = markup_style("");
    static_assert(s.is_default());
    SUCCEED();
}

// ── ct_parse_markup: full compile-time parser ───────────────────────────

TEST(ConstexprMarkupTest, ParsePlainText) {
    constexpr auto m = ct_parse_markup("Hello World");
    static_assert(m.count == 1);
    static_assert(m.fragments[0].offset == 0);
    static_assert(m.fragments[0].length == 11);
    static_assert(m.fragments[0].sty.is_default());
    EXPECT_EQ(m.count, 1u);
}

TEST(ConstexprMarkupTest, ParseSingleTag) {
    constexpr auto m = ct_parse_markup("[bold]Hello[/bold]");
    static_assert(m.count == 1);
    static_assert(m.fragments[0].offset == 6);
    static_assert(m.fragments[0].length == 5);
    static_assert(has_attr(m.fragments[0].sty.attrs, attr::bold));
    EXPECT_EQ(m.count, 1u);
}

TEST(ConstexprMarkupTest, ParseMixedContent) {
    constexpr auto m = ct_parse_markup("[bold]Bold[/] Normal");
    static_assert(m.count == 2);
    // "Bold" — bold style
    static_assert(m.fragments[0].offset == 6);
    static_assert(m.fragments[0].length == 4);
    static_assert(has_attr(m.fragments[0].sty.attrs, attr::bold));
    // " Normal" — default style
    static_assert(m.fragments[1].offset == 13);
    static_assert(m.fragments[1].length == 7);
    static_assert(m.fragments[1].sty.is_default());
    EXPECT_EQ(m.count, 2u);
}

TEST(ConstexprMarkupTest, ParseNestedTags) {
    constexpr auto m = ct_parse_markup("[bold][red]Hello[/red][/bold]");
    static_assert(m.count == 1);
    static_assert(has_attr(m.fragments[0].sty.attrs, attr::bold));
    static_assert(m.fragments[0].sty.fg == colors::red);
    SUCCEED();
}

TEST(ConstexprMarkupTest, ParseCompoundTag) {
    constexpr auto m = ct_parse_markup("[bold red]Error[/]");
    static_assert(m.count == 1);
    static_assert(has_attr(m.fragments[0].sty.attrs, attr::bold));
    static_assert(m.fragments[0].sty.fg == colors::red);
    SUCCEED();
}

TEST(ConstexprMarkupTest, ParseMultipleFragments) {
    constexpr auto m = ct_parse_markup("[red]R[/][green]G[/][blue]B[/]");
    static_assert(m.count == 3);
    static_assert(m.fragments[0].sty.fg == colors::red);
    static_assert(m.fragments[1].sty.fg == colors::green);
    static_assert(m.fragments[2].sty.fg == colors::blue);
    EXPECT_EQ(m.count, 3u);
}

TEST(ConstexprMarkupTest, ParseResetTag) {
    constexpr auto m = ct_parse_markup("[bold]A[/]B");
    static_assert(m.count == 2);
    static_assert(has_attr(m.fragments[0].sty.attrs, attr::bold));
    static_assert(m.fragments[1].sty.is_default());
    SUCCEED();
}

// ── Extended inline tags ─────────────────────────────────────────────

TEST(ConstexprMarkupTest, DoubleUnderline) {
    constexpr auto s = markup_style("double_underline");
    static_assert(has_attr(s.attrs, attr::double_underline));
    SUCCEED();
}

TEST(ConstexprMarkupTest, Overline) {
    constexpr auto s = markup_style("overline");
    static_assert(has_attr(s.attrs, attr::overline));
    SUCCEED();
}

TEST(ConstexprMarkupTest, SuperscriptSubscript) {
    constexpr auto s1 = markup_style("superscript");
    constexpr auto s2 = markup_style("subscript");
    static_assert(has_attr(s1.attrs, attr::superscript));
    static_assert(has_attr(s2.attrs, attr::subscript));
    SUCCEED();
}

TEST(ConstexprMarkupTest, NegationTags) {
    constexpr auto s = markup_style("bold not_bold");
    static_assert(!has_attr(s.attrs, attr::bold));
    constexpr auto s2 = markup_style("italic not_italic");
    static_assert(!has_attr(s2.attrs, attr::italic));
    constexpr auto s3 = markup_style("strike not_strike");
    static_assert(!has_attr(s3.attrs, attr::strike));
    constexpr auto s4 = markup_style("overline not_overline");
    static_assert(!has_attr(s4.attrs, attr::overline));
    SUCCEED();
}

TEST(ConstexprMarkupTest, RgbFunc) {
    constexpr auto s = markup_style("rgb(255,128,0)");
    static_assert(s.fg == color::from_rgb(255, 128, 0));
    static_assert(s.bg.is_default());
    SUCCEED();
}

TEST(ConstexprMarkupTest, OnRgbFunc) {
    constexpr auto s = markup_style("on_rgb(0,0,80)");
    static_assert(s.bg == color::from_rgb(0, 0, 80));
    static_assert(s.fg.is_default());
    SUCCEED();
}

TEST(ConstexprMarkupTest, Color256Func) {
    constexpr auto s = markup_style("color256(208)");
    static_assert(s.fg == color::from_index_256(208));
    SUCCEED();
}

TEST(ConstexprMarkupTest, OnColor256Func) {
    constexpr auto s = markup_style("on_color256(17)");
    static_assert(s.bg == color::from_index_256(17));
    SUCCEED();
}

TEST(ConstexprMarkupTest, DefaultResetsFg) {
    constexpr auto s = markup_style("red default");
    static_assert(s.fg.is_default());
    SUCCEED();
}

TEST(ConstexprMarkupTest, OnDefaultResetsBg) {
    constexpr auto s = markup_style("on_blue on_default");
    static_assert(s.bg.is_default());
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticError) {
    constexpr auto s = markup_style("error");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::red);
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticWarning) {
    constexpr auto s = markup_style("warning");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::yellow);
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticSuccess) {
    constexpr auto s = markup_style("success");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::green);
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticInfo) {
    constexpr auto s = markup_style("info");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::cyan);
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticDebugMuted) {
    constexpr auto s1 = markup_style("debug");
    constexpr auto s2 = markup_style("muted");
    static_assert(has_attr(s1.attrs, attr::dim));
    static_assert(has_attr(s2.attrs, attr::dim));
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticHighlight) {
    constexpr auto s = markup_style("highlight");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::black);
    static_assert(s.bg == colors::yellow);
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticCodeInline) {
    constexpr auto s = markup_style("code_inline");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::cyan);
    static_assert(s.bg == color::from_rgb(0x1e, 0x1e, 0x2e));
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticUrlPath) {
    constexpr auto s1 = markup_style("url");
    static_assert(has_attr(s1.attrs, attr::underline));
    static_assert(s1.fg == colors::bright_blue);
    constexpr auto s2 = markup_style("path");
    static_assert(has_attr(s2.attrs, attr::italic));
    static_assert(s2.fg == colors::bright_green);
    SUCCEED();
}

TEST(ConstexprMarkupTest, SemanticKey) {
    constexpr auto s = markup_style("key");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::white);
    static_assert(s.bg == color::from_rgb(0x3a, 0x3a, 0x4a));
    SUCCEED();
}

TEST(ConstexprMarkupTest, BadgeRed) {
    constexpr auto s = markup_style("badge.red");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::white);
    static_assert(s.bg == colors::red);
    SUCCEED();
}

TEST(ConstexprMarkupTest, BadgeYellow) {
    constexpr auto s = markup_style("badge.yellow");
    static_assert(has_attr(s.attrs, attr::bold));
    static_assert(s.fg == colors::black);
    static_assert(s.bg == colors::yellow);
    SUCCEED();
}

// ── compile_markup() tests ───────────────────────────────────────────

TEST(ConstexprMarkupTest, CompileMarkupBr) {
    constexpr auto plan = compile_markup("[br]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::line_break);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupSpacing) {
    constexpr auto plan = compile_markup("[sp=5]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::spacing);
    static_assert(plan.fragments[0].block.level == 5);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupBox) {
    constexpr auto plan = compile_markup("[box]Hi[/box]");
    static_assert(plan.count == 3);
    static_assert(plan.fragments[0].block.kind == block_kind::box_open);
    static_assert(plan.fragments[1].block.kind == block_kind::none);  // text "Hi"
    static_assert(plan.fragments[2].block.kind == block_kind::box_close);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupBoxHeavy) {
    constexpr auto plan = compile_markup("[box.heavy]X[/box]");
    static_assert(plan.count == 3);
    static_assert(plan.fragments[0].block.kind == block_kind::box_open);
    static_assert(plan.fragments[0].block.border == 3);  // heavy
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupHeading) {
    constexpr auto plan = compile_markup("[h1]Title[/h1]");
    static_assert(plan.count == 3);
    static_assert(plan.fragments[0].block.kind == block_kind::heading);
    static_assert(plan.fragments[0].block.level == 1);
    static_assert(plan.fragments[1].block.kind == block_kind::none);  // text
    static_assert(plan.fragments[2].block.kind == block_kind::heading_close);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupRule) {
    constexpr auto plan = compile_markup("[rule]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::rule);
    static_assert(plan.fragments[0].block.title_length == 0);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupRuleWithTitle) {
    constexpr auto plan = compile_markup("[rule=Section]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::rule);
    static_assert(plan.fragments[0].block.title_length == 7);  // "Section"
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupProgress) {
    constexpr auto plan = compile_markup("[progress=75]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::progress_bar);
    static_assert(plan.fragments[0].block.numerator == 75);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupBar) {
    constexpr auto plan = compile_markup("[bar=3/5]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::fraction_bar);
    static_assert(plan.fragments[0].block.numerator == 3);
    static_assert(plan.fragments[0].block.denominator == 5);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupMixed) {
    constexpr auto plan = compile_markup("[bold]A[/][box]B[/box]");
    static_assert(plan.count == 4);
    static_assert(plan.fragments[0].block.kind == block_kind::none);  // "A" bold
    static_assert(plan.fragments[1].block.kind == block_kind::box_open);
    static_assert(plan.fragments[2].block.kind == block_kind::none);  // "B"
    static_assert(plan.fragments[3].block.kind == block_kind::box_close);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupEmpty) {
    constexpr auto plan = compile_markup("");
    static_assert(plan.count == 0);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupEmoji) {
    constexpr auto plan = compile_markup("[emoji=fire]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::emoji);
    static_assert(plan.fragments[0].block.name_length == 4);  // "fire"
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupLink) {
    constexpr auto plan = compile_markup("[link=https://x.com]click[/link]");
    static_assert(plan.count == 3);
    static_assert(plan.fragments[0].block.kind == block_kind::link_open);
    static_assert(plan.fragments[0].block.url_length == 13);  // "https://x.com"
    static_assert(plan.fragments[1].block.kind == block_kind::none);  // "click"
    static_assert(plan.fragments[2].block.kind == block_kind::link_close);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupQuote) {
    constexpr auto plan = compile_markup("[quote]text[/quote]");
    static_assert(plan.count == 3);
    static_assert(plan.fragments[0].block.kind == block_kind::quote_open);
    static_assert(plan.fragments[1].block.kind == block_kind::none);
    static_assert(plan.fragments[2].block.kind == block_kind::quote_close);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupIndent) {
    constexpr auto plan = compile_markup("[indent=8]text[/indent]");
    static_assert(plan.count == 3);
    static_assert(plan.fragments[0].block.kind == block_kind::indent_open);
    static_assert(plan.fragments[0].block.level == 8);
    static_assert(plan.fragments[2].block.kind == block_kind::indent_close);
    SUCCEED();
}

TEST(ConstexprMarkupTest, CompileMarkupHr) {
    constexpr auto plan = compile_markup("[hr]");
    static_assert(plan.count == 1);
    static_assert(plan.fragments[0].block.kind == block_kind::rule);
    SUCCEED();
}

TEST(ConstexprMarkupTest, FragmentTextExtraction) {
    constexpr auto m = ct_parse_markup("[bold]Hello[/] World");
    constexpr const char* src = "[bold]Hello[/] World";
    // Verify we can extract text from fragments at runtime
    std::string_view full(src);
    auto f0 = full.substr(m.fragments[0].offset, m.fragments[0].length);
    auto f1 = full.substr(m.fragments[1].offset, m.fragments[1].length);
    EXPECT_EQ(f0, "Hello");
    EXPECT_EQ(f1, " World");
}
