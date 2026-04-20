/**
 * @file paragraph_test.cpp
 * @brief Tests for paragraph element factories.
 */

#include "tapiru/core/console.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/paragraph.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── Helper ──────────────────────────────────────────────────────────────

class virtual_terminal {
  public:
    [[nodiscard]] console make_console() {
        console_config cfg;
        cfg.sink = [this](std::string_view data) { buffer_ += data; };
        cfg.depth = color_depth::none;
        cfg.no_color = true;
        return console(cfg);
    }
    [[nodiscard]] const std::string &raw() const noexcept { return buffer_; }
    void clear() { buffer_.clear(); }

  private:
    std::string buffer_;
};

static std::string render_elem(element e, uint32_t width = 40) {
    virtual_terminal vt;
    auto con = vt.make_console();
    con.print_widget(e, width);
    return vt.raw();
}

// ── Tests ───────────────────────────────────────────────────────────────

TEST(ParagraphTest, WordWrapBasic) {
    auto e = make_paragraph("The quick brown fox jumps over the lazy dog");
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e, 20);
    EXPECT_FALSE(out.empty());
}

TEST(ParagraphTest, WordWrapLongWord) {
    auto e = make_paragraph("supercalifragilisticexpialidocious is a word");
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e, 15);
    EXPECT_FALSE(out.empty());
}

TEST(ParagraphTest, PreserveNewlines) {
    auto e = make_paragraph("Line one\nLine two\nLine three");
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e, 40);
    EXPECT_TRUE(out.find("Line one") != std::string::npos);
    EXPECT_TRUE(out.find("Line two") != std::string::npos);
    EXPECT_TRUE(out.find("Line three") != std::string::npos);
}

TEST(ParagraphTest, EmptyParagraph) {
    auto e = make_paragraph("");
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e, 40);
    // Should not crash
    EXPECT_TRUE(true);
}

TEST(ParagraphTest, JustifyAlign) {
    auto e = make_paragraph_justify("The quick brown fox jumps over the lazy dog");
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e, 40);
    EXPECT_FALSE(out.empty());
}

TEST(ParagraphTest, ParagraphInLayout) {
    auto rb = rows_builder();
    rb.add(text_builder("Title"));
    auto para = make_paragraph("Some body text that should wrap nicely.");
    rb.add(std::move(para));
    element layout = std::move(rb);
    auto out = render_elem(layout, 30);
    EXPECT_TRUE(out.find("Title") != std::string::npos);
}
