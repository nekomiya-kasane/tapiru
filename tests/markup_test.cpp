#include "tapiru/text/markup.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── Plain text (no markup) ──────────────────────────────────────────────

TEST(MarkupTest, PlainText) {
    auto frags = parse_markup("Hello World");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "Hello World");
    EXPECT_TRUE(frags[0].sty.is_default());
}

TEST(MarkupTest, EmptyString) {
    auto frags = parse_markup("");
    EXPECT_TRUE(frags.empty());
}

// ── Single tag ──────────────────────────────────────────────────────────

TEST(MarkupTest, BoldTag) {
    auto frags = parse_markup("[bold]Hello[/bold]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "Hello");
    EXPECT_TRUE(has_attr(frags[0].sty.attrs, attr::bold));
}

TEST(MarkupTest, RedForeground) {
    auto frags = parse_markup("[red]Error[/red]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "Error");
    EXPECT_EQ(frags[0].sty.fg, colors::red);
}

TEST(MarkupTest, OnBlueBackground) {
    auto frags = parse_markup("[on_blue]Info[/on_blue]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "Info");
    EXPECT_EQ(frags[0].sty.bg, colors::blue);
}

// ── Compound tags ───────────────────────────────────────────────────────

TEST(MarkupTest, CompoundTag) {
    auto frags = parse_markup("[bold red]Warning[/bold red]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "Warning");
    EXPECT_TRUE(has_attr(frags[0].sty.attrs, attr::bold));
    EXPECT_EQ(frags[0].sty.fg, colors::red);
}

TEST(MarkupTest, CompoundFgBg) {
    auto frags = parse_markup("[white on_red]Alert[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.fg, colors::white);
    EXPECT_EQ(frags[0].sty.bg, colors::red);
}

// ── Nested tags ─────────────────────────────────────────────────────────

TEST(MarkupTest, NestedTags) {
    auto frags = parse_markup("[bold]Hello [red]World[/red][/bold]");
    ASSERT_EQ(frags.size(), 2u);

    EXPECT_EQ(frags[0].text, "Hello ");
    EXPECT_TRUE(has_attr(frags[0].sty.attrs, attr::bold));
    EXPECT_TRUE(frags[0].sty.fg.is_default());

    EXPECT_EQ(frags[1].text, "World");
    EXPECT_TRUE(has_attr(frags[1].sty.attrs, attr::bold));
    EXPECT_EQ(frags[1].sty.fg, colors::red);
}

// ── Reset tag [/] ───────────────────────────────────────────────────────

TEST(MarkupTest, ResetTag) {
    auto frags = parse_markup("[bold red]Hello[/] World");
    ASSERT_EQ(frags.size(), 2u);

    EXPECT_EQ(frags[0].text, "Hello");
    EXPECT_TRUE(has_attr(frags[0].sty.attrs, attr::bold));
    EXPECT_EQ(frags[0].sty.fg, colors::red);

    EXPECT_EQ(frags[1].text, " World");
    EXPECT_TRUE(frags[1].sty.is_default());
}

// ── Escaped bracket ─────────────────────────────────────────────────────

TEST(MarkupTest, EscapedBracket) {
    auto frags = parse_markup("Use [[bold] for bold");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "Use [bold] for bold");
}

// ── RGB hex colors ──────────────────────────────────────────────────────

TEST(MarkupTest, HexFgColor) {
    auto frags = parse_markup("[#FF8000]Orange[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.fg, color::from_rgb(0xFF, 0x80, 0x00));
}

TEST(MarkupTest, HexBgColor) {
    auto frags = parse_markup("[on_#00FF00]Green BG[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.bg, color::from_rgb(0x00, 0xFF, 0x00));
}

// ── Bright colors ───────────────────────────────────────────────────────

TEST(MarkupTest, BrightRed) {
    auto frags = parse_markup("[bright_red]Bright[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.fg, colors::bright_red);
}

// ── All attributes ──────────────────────────────────────────────────────

TEST(MarkupTest, AllAttributes) {
    auto frags = parse_markup("[bold dim italic underline blink reverse hidden strike]X[/]");
    ASSERT_EQ(frags.size(), 1u);
    auto a = frags[0].sty.attrs;
    EXPECT_TRUE(has_attr(a, attr::bold));
    EXPECT_TRUE(has_attr(a, attr::dim));
    EXPECT_TRUE(has_attr(a, attr::italic));
    EXPECT_TRUE(has_attr(a, attr::underline));
    EXPECT_TRUE(has_attr(a, attr::blink));
    EXPECT_TRUE(has_attr(a, attr::reverse));
    EXPECT_TRUE(has_attr(a, attr::hidden));
    EXPECT_TRUE(has_attr(a, attr::strike));
}

// ── Mixed text and tags ─────────────────────────────────────────────────

TEST(MarkupTest, MixedContent) {
    auto frags = parse_markup("Hello [bold]World[/bold]!");
    ASSERT_EQ(frags.size(), 3u);

    EXPECT_EQ(frags[0].text, "Hello ");
    EXPECT_TRUE(frags[0].sty.is_default());

    EXPECT_EQ(frags[1].text, "World");
    EXPECT_TRUE(has_attr(frags[1].sty.attrs, attr::bold));

    EXPECT_EQ(frags[2].text, "!");
    EXPECT_TRUE(frags[2].sty.is_default());
}

// ── Unclosed bracket treated as literal ─────────────────────────────────

TEST(MarkupTest, UnclosedBracket) {
    auto frags = parse_markup("Hello [world");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "Hello [world");
}

// ── strip_markup ────────────────────────────────────────────────────────

TEST(StripMarkupTest, PlainText) {
    EXPECT_EQ(strip_markup("Hello"), "Hello");
}

TEST(StripMarkupTest, RemovesTags) {
    EXPECT_EQ(strip_markup("[bold]Hello[/bold] World"), "Hello World");
}

TEST(StripMarkupTest, PreservesEscapedBrackets) {
    EXPECT_EQ(strip_markup("Use [[bold] syntax"), "Use [bold] syntax");
}

TEST(StripMarkupTest, NestedTags) {
    EXPECT_EQ(strip_markup("[bold][red]Hello[/red][/bold]"), "Hello");
}

TEST(StripMarkupTest, Empty) {
    EXPECT_EQ(strip_markup(""), "");
}

TEST(StripMarkupTest, OnlyTags) {
    EXPECT_EQ(strip_markup("[bold][/bold]"), "");
}

// ── Extended inline tags (runtime) ──────────────────────────────────

TEST(MarkupTest, NewAttributes) {
    auto frags = parse_markup("[double_underline overline superscript subscript]X[/]");
    ASSERT_EQ(frags.size(), 1u);
    auto a = frags[0].sty.attrs;
    EXPECT_TRUE(has_attr(a, attr::double_underline));
    EXPECT_TRUE(has_attr(a, attr::overline));
    EXPECT_TRUE(has_attr(a, attr::superscript));
    EXPECT_TRUE(has_attr(a, attr::subscript));
}

TEST(MarkupTest, NegationTag) {
    auto frags = parse_markup("[bold]A[not_bold]B[/]");
    ASSERT_EQ(frags.size(), 2u);
    EXPECT_TRUE(has_attr(frags[0].sty.attrs, attr::bold));
    EXPECT_FALSE(has_attr(frags[1].sty.attrs, attr::bold));
}

TEST(MarkupTest, RgbFuncFg) {
    auto frags = parse_markup("[rgb(255,0,0)]X[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.fg, color::from_rgb(255, 0, 0));
}

TEST(MarkupTest, OnRgbFuncBg) {
    auto frags = parse_markup("[on_rgb(0,128,255)]X[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.bg, color::from_rgb(0, 128, 255));
}

TEST(MarkupTest, Color256Fg) {
    auto frags = parse_markup("[color256(208)]X[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.fg, color::from_index_256(208));
}

TEST(MarkupTest, OnColor256Bg) {
    auto frags = parse_markup("[on_color256(17)]X[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].sty.bg, color::from_index_256(17));
}

TEST(MarkupTest, DefaultResetsFg) {
    auto frags = parse_markup("[red]A[default]B[/]");
    ASSERT_EQ(frags.size(), 2u);
    EXPECT_EQ(frags[0].sty.fg, colors::red);
    EXPECT_TRUE(frags[1].sty.fg.is_default());
}

TEST(MarkupTest, SemanticError) {
    auto frags = parse_markup("[error]E[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_TRUE(has_attr(frags[0].sty.attrs, attr::bold));
    EXPECT_EQ(frags[0].sty.fg, colors::red);
}

TEST(MarkupTest, SemanticBadgeGreen) {
    auto frags = parse_markup("[badge.green]OK[/]");
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_TRUE(has_attr(frags[0].sty.attrs, attr::bold));
    EXPECT_EQ(frags[0].sty.fg, colors::white);
    EXPECT_EQ(frags[0].sty.bg, colors::green);
}

TEST(StripMarkupTest, StripRgbTag) {
    EXPECT_EQ(strip_markup("[rgb(1,2,3)]X[/]"), "X");
}

TEST(StripMarkupTest, StripSemanticTag) {
    EXPECT_EQ(strip_markup("[error]E[/][success]S[/]"), "ES");
}
