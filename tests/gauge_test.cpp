/**
 * @file gauge_test.cpp
 * @brief Tests for gauge element factories.
 */

#include "tapiru/core/console.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/gauge.h"

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

TEST(GaugeTest, GaugeZero) {
    auto e = make_gauge(0.0f);
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e);
    EXPECT_FALSE(out.empty());
}

TEST(GaugeTest, GaugeFull) {
    auto e = make_gauge(1.0f);
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e);
    EXPECT_FALSE(out.empty());
}

TEST(GaugeTest, GaugeHalf) {
    auto e = make_gauge(0.5f);
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e);
    EXPECT_FALSE(out.empty());
}

TEST(GaugeTest, GaugeCustomStyle) {
    style filled;
    filled.fg = color::from_rgb(0, 255, 0);
    style remaining;
    remaining.fg = color::from_rgb(100, 100, 100);
    auto e = make_gauge(0.7f, filled, remaining);
    EXPECT_FALSE(e.empty());
}

TEST(GaugeTest, GaugeDirection) {
    auto e = make_gauge_direction(0.5f, gauge_direction::vertical);
    EXPECT_FALSE(e.empty());
    auto out = render_elem(e, 5);
    EXPECT_FALSE(out.empty());
}

TEST(GaugeTest, GaugeInLayout) {
    auto rb = rows_builder();
    rb.add(text_builder("Progress:"));
    auto gauge = make_gauge(0.75f);
    // Wrap gauge in a sized_box for layout
    rb.add(std::move(gauge));
    element layout = std::move(rb);
    auto out = render_elem(layout, 30);
    EXPECT_TRUE(out.find("Progress") != std::string::npos);
}

TEST(GaugeTest, GaugeClampAboveOne) {
    auto e = make_gauge(1.5f);
    EXPECT_FALSE(e.empty());
}

TEST(GaugeTest, GaugeClampBelowZero) {
    auto e = make_gauge(-0.5f);
    EXPECT_FALSE(e.empty());
}
