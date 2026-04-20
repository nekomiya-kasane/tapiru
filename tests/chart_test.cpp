/**
 * @file chart_test.cpp
 * @brief Tests for braille_grid, line_chart, bar_chart, scatter chart widgets.
 */

#include <gtest/gtest.h>
#include "tapiru/widgets/chart.h"
#include "tapiru/core/console.h"

using namespace tapiru;

class virtual_terminal {
public:
    [[nodiscard]] console make_console(bool color = false, uint32_t width = 80) {
        console_config cfg;
        cfg.sink = [this](std::string_view data) { buffer_ += data; };
        cfg.depth = color ? color_depth::true_color : color_depth::none;
        cfg.force_color = color;
        cfg.no_color = !color;
        return console(cfg);
    }
    [[nodiscard]] const std::string& raw() const noexcept { return buffer_; }
    void clear() { buffer_.clear(); }
private:
    std::string buffer_;
};

// ── braille_grid ───────────────────────────────────────────────────────

TEST(BrailleGridTest, Dimensions) {
    braille_grid g(10, 5);
    EXPECT_EQ(g.char_width(), 10u);
    EXPECT_EQ(g.char_height(), 5u);
    EXPECT_EQ(g.dot_width(), 20u);
    EXPECT_EQ(g.dot_height(), 20u);
}

TEST(BrailleGridTest, EmptyGridRendersBlankBraille) {
    braille_grid g(2, 1);
    auto s = g.render();
    // Empty braille char is U+2800 = 0xE2 0xA0 0x80
    EXPECT_FALSE(s.empty());
    // Should be 2 braille chars (6 bytes)
    EXPECT_EQ(s.size(), 6u);
}

TEST(BrailleGridTest, SetDotProducesBraille) {
    braille_grid g(1, 1);
    g.set(0, 0);  // top-left dot
    auto s = g.render();
    // U+2800 + 0x01 = U+2801
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(static_cast<uint8_t>(s[0]), 0xE2u);
    EXPECT_EQ(static_cast<uint8_t>(s[1]), 0xA0u);
    EXPECT_EQ(static_cast<uint8_t>(s[2]), 0x81u);
}

TEST(BrailleGridTest, ClearResetsGrid) {
    braille_grid g(1, 1);
    g.set(0, 0);
    g.clear();
    auto s = g.render();
    // Should be blank braille U+2800
    EXPECT_EQ(static_cast<uint8_t>(s[2]), 0x80u);
}

TEST(BrailleGridTest, OutOfBoundsIgnored) {
    braille_grid g(1, 1);
    g.set(999, 999);  // should not crash
    auto s = g.render();
    EXPECT_EQ(s.size(), 3u);
}

TEST(BrailleGridTest, MultiRowRender) {
    braille_grid g(2, 2);
    auto s = g.render();
    // 2 rows, each with 2 braille chars (6 bytes), separated by newline
    // Total: 6 + 1 + 6 = 13
    EXPECT_EQ(s.size(), 13u);
    EXPECT_NE(s.find('\n'), std::string::npos);
}

// ── line_chart_builder ─────────────────────────────────────────────────

TEST(LineChartTest, RendersNonEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    std::vector<float> data = {0, 1, 2, 3, 4, 5, 4, 3, 2, 1};
    con.print_widget(line_chart_builder(data, 10, 3), 80);
    EXPECT_FALSE(vt.raw().empty());
}

TEST(LineChartTest, EmptyDataRendersEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    con.print_widget(line_chart_builder({}, 10, 3), 80);
    // Should not crash
}

TEST(LineChartTest, SingleValueRendersOk) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    con.print_widget(line_chart_builder({42.0f}, 5, 2), 80);
    EXPECT_FALSE(vt.raw().empty());
}

// ── bar_chart_builder ──────────────────────────────────────────────────

TEST(BarChartTest, RendersBlockChars) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    std::vector<float> data = {1, 3, 5, 2, 4};
    con.print_widget(bar_chart_builder(data, 4), 80);
    auto& out = vt.raw();
    EXPECT_FALSE(out.empty());
}

TEST(BarChartTest, WithLabels) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    std::vector<float> data = {3, 7, 5};
    auto bc = bar_chart_builder(data, 4).labels({"A", "B", "C"});
    con.print_widget(std::move(bc), 80);
    auto& out = vt.raw();
    EXPECT_TRUE(out.find("A") != std::string::npos);
}

TEST(BarChartTest, EmptyDataRendersEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    con.print_widget(bar_chart_builder({}, 4), 80);
}

// ── scatter_builder ────────────────────────────────────────────────────

TEST(ScatterTest, RendersNonEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    std::vector<scatter_point> pts = {{0, 0}, {1, 1}, {2, 0.5f}, {3, 2}};
    con.print_widget(scatter_builder(pts, 10, 3), 80);
    EXPECT_FALSE(vt.raw().empty());
}

TEST(ScatterTest, EmptyPointsRendersEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    con.print_widget(scatter_builder({}, 10, 3), 80);
}

TEST(ScatterTest, SinglePoint) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    con.print_widget(scatter_builder({{5.0f, 5.0f}}, 5, 2), 80);
    EXPECT_FALSE(vt.raw().empty());
}
