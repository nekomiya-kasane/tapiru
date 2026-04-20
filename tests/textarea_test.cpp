/**
 * @file textarea_test.cpp
 * @brief Tests for textarea_builder: line numbers, relative numbers, gutter style,
 *        block cursor, selection, search matches, EOF style, scroll offset.
 */

#include <gtest/gtest.h>

#include "tapiru/widgets/textarea.h"
#include "tapiru/core/console.h"
#include "tapiru/core/element.h"
#include "tapiru/widgets/builders.h"

using namespace tapiru;

// ── Helper ──────────────────────────────────────────────────────────────

class virtual_terminal {
public:
    [[nodiscard]] console make_console() {
        console_config cfg;
        cfg.sink = [this](std::string_view data) { buffer_ += data; };
        cfg.depth = color_depth::true_color;
        return console(cfg);
    }
    [[nodiscard]] const std::string& raw() const noexcept { return buffer_; }
    void clear() { buffer_.clear(); }
private:
    std::string buffer_;
};

static std::string render_textarea(element e, uint32_t width = 60) {
    virtual_terminal vt;
    auto con = vt.make_console();
    con.print_widget(e, width);
    return vt.raw();
}

// ── Basic rendering ─────────────────────────────────────────────────────

TEST(TextareaTest, BasicRender) {
    std::string text = "Hello\nWorld\nFoo";
    int cr = 0, cc = 0, sr = 0;
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(5).border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    EXPECT_FALSE(out.empty());
    // Should contain "Hello" and "World"
    EXPECT_NE(out.find("Hello"), std::string::npos);
    EXPECT_NE(out.find("World"), std::string::npos);
}

// ── Line numbers ────────────────────────────────────────────────────────

TEST(TextareaTest, LineNumbers) {
    std::string text = "aaa\nbbb\nccc\nddd\neee";
    int cr = 0, cc = 0, sr = 0;
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(5).show_line_numbers()
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Should contain line numbers 1-5
    EXPECT_NE(out.find("1"), std::string::npos);
    EXPECT_NE(out.find("2"), std::string::npos);
    EXPECT_NE(out.find("3"), std::string::npos);
}

// ── Relative line numbers ───────────────────────────────────────────────

TEST(TextareaTest, RelativeLineNumbers) {
    std::string text = "aaa\nbbb\nccc\nddd\neee";
    int cr = 2, cc = 0, sr = 0;  // cursor on line 2 (0-indexed)
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(5).relative_line_numbers()
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Cursor line shows absolute number (3), others show relative distance
    EXPECT_NE(out.find("3"), std::string::npos);  // absolute for cursor line
    // Lines 0,1 are distance 2,1 from cursor; lines 3,4 are distance 1,2
}

// ── Gutter style ────────────────────────────────────────────────────────

TEST(TextareaTest, GutterStyle) {
    std::string text = "Hello\nWorld";
    int cr = 0, cc = 0, sr = 0;
    style gutter_sty{color::from_rgb(100, 100, 100), color::default_color()};
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(3).show_line_numbers()
      .line_number_style(gutter_sty)
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Gutter should have the specified fg color (100,100,100)
    // Check for the ANSI sequence 38;2;100;100;100
    EXPECT_NE(out.find("38;2;100;100;100"), std::string::npos);
}

// ── Block cursor ────────────────────────────────────────────────────────

TEST(TextareaTest, BlockCursor) {
    std::string text = "Hello\nWorld";
    int cr = 0, cc = 2, sr = 0;  // cursor at 'l' in "Hello"
    style cursor_char{color::from_rgb(0, 0, 0), color::from_rgb(255, 255, 255)};
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(3).show_cursor()
      .cursor_char_style(cursor_char)
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Should contain the cursor char bg color (255,255,255)
    EXPECT_NE(out.find("48;2;255;255;255"), std::string::npos);
}

// ── Selection ───────────────────────────────────────────────────────────

TEST(TextareaTest, Selection) {
    std::string text = "Hello World";
    int cr = 0, cc = 0, sr = 0;
    text_range sel{0, 2, 0, 5};  // select "llo" (cols 2-5)
    style sel_sty{color::from_rgb(255, 255, 255), color::from_rgb(0, 0, 200)};
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(2).selection(&sel).selection_style(sel_sty)
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Should contain selection bg color (0,0,200)
    EXPECT_NE(out.find("48;2;0;0;200"), std::string::npos);
}

// ── Search matches ──────────────────────────────────────────────────────

TEST(TextareaTest, SearchMatches) {
    std::string text = "foo bar foo baz";
    int cr = 0, cc = 0, sr = 0;
    std::vector<text_range> matches = {
        {0, 0, 0, 3},   // first "foo"
        {0, 8, 0, 11},  // second "foo"
    };
    style match_sty{color::from_rgb(0, 0, 0), color::from_rgb(255, 255, 0)};
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(2).matches(&matches).match_style(match_sty)
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Should contain match bg color (255,255,0)
    EXPECT_NE(out.find("48;2;255;255;0"), std::string::npos);
}

// ── EOF style ───────────────────────────────────────────────────────────

TEST(TextareaTest, EofStyle) {
    std::string text = "Hello";
    int cr = 0, cc = 0, sr = 0;
    style eof_sty{color::from_rgb(80, 80, 80), color::default_color()};
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(3).eof_style(eof_sty)
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Should contain ~ and the eof fg color (80,80,80)
    EXPECT_NE(out.find("~"), std::string::npos);
    EXPECT_NE(out.find("38;2;80;80;80"), std::string::npos);
}

// ── Scroll offset ───────────────────────────────────────────────────────

TEST(TextareaTest, ScrollOffset) {
    std::string text = "line0\nline1\nline2\nline3\nline4";
    int cr = 3, cc = 0, sr = 2;  // scroll starts at line 2
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(3).border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Should show line2, line3, line4 (not line0, line1)
    EXPECT_NE(out.find("line2"), std::string::npos);
    EXPECT_NE(out.find("line3"), std::string::npos);
    EXPECT_EQ(out.find("line0"), std::string::npos);
    EXPECT_EQ(out.find("line1"), std::string::npos);
}

// ── Cursor line highlight ───────────────────────────────────────────────

TEST(TextareaTest, CursorLineHighlight) {
    std::string text = "aaa\nbbb\nccc";
    int cr = 1, cc = 0, sr = 0;
    style cursor_line_sty{color::default_color(), color::from_rgb(30, 30, 50)};
    style text_sty{color::from_rgb(200, 200, 200), color::default_color()};
    textarea_builder ta;
    ta.text(&text).cursor_row(&cr).cursor_col(&cc).scroll_row(&sr)
      .width(20).height(3).text_style(text_sty).cursor_style(cursor_line_sty)
      .border(border_style::none);
    element e = std::move(ta);
    auto out = render_textarea(e);
    // Cursor line (bbb) should have bg color (30,30,50)
    EXPECT_NE(out.find("48;2;30;30;50"), std::string::npos);
}
