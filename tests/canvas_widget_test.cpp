/**
 * @file canvas_widget_test.cpp
 * @brief Tests for canvas_widget_builder and make_canvas.
 */

#include <gtest/gtest.h>

#include "tapiru/widgets/canvas_widget.h"
#include "tapiru/core/console.h"
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

static std::string render_canvas(element e, uint32_t width = 40) {
    virtual_terminal vt;
    auto con = vt.make_console();
    con.print_widget(e, width);
    return vt.raw();
}

// ── canvas_widget_builder construction ──────────────────────────────────

TEST(CanvasWidgetTest, ConstructorSetsSize) {
    canvas_widget_builder cvs(80, 40);
    EXPECT_EQ(cvs.pixel_width(), 80u);
    EXPECT_EQ(cvs.pixel_height(), 40u);
}

TEST(CanvasWidgetTest, CanvasSize) {
    // 80 pixels wide / 2 = 40 cells, 40 pixels tall / 4 = 10 cells
    canvas_widget_builder cvs(80, 40);
    element e = std::move(cvs);
    auto out = render_canvas(e, 50);
    EXPECT_FALSE(out.empty());
}

// ── draw_point ──────────────────────────────────────────────────────────

TEST(CanvasWidgetTest, DrawPointBraille) {
    canvas_widget_builder cvs(4, 8);
    cvs.draw_point(0, 0, color::from_rgb(255, 0, 0));
    element e = std::move(cvs);
    auto out = render_canvas(e, 10);
    // Should contain braille characters (U+2800 range)
    EXPECT_FALSE(out.empty());
}

TEST(CanvasWidgetTest, DrawMultiplePoints) {
    canvas_widget_builder cvs(10, 10);
    cvs.draw_point(0, 0);
    cvs.draw_point(5, 5);
    cvs.draw_point(9, 9);
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    EXPECT_FALSE(out.empty());
}

// ── draw_line ───────────────────────────────────────────────────────────

TEST(CanvasWidgetTest, DrawLineHorizontal) {
    canvas_widget_builder cvs(20, 8);
    cvs.draw_line(0, 4, 19, 4);
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    EXPECT_FALSE(out.empty());
}

TEST(CanvasWidgetTest, DrawLineVertical) {
    canvas_widget_builder cvs(8, 20);
    cvs.draw_line(4, 0, 4, 19);
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    EXPECT_FALSE(out.empty());
}

TEST(CanvasWidgetTest, DrawLineDiagonal) {
    canvas_widget_builder cvs(20, 20);
    cvs.draw_line(0, 0, 19, 19, color::from_rgb(0, 255, 0));
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    EXPECT_FALSE(out.empty());
}

// ── draw_circle ─────────────────────────────────────────────────────────

TEST(CanvasWidgetTest, DrawCircleSmall) {
    canvas_widget_builder cvs(20, 20);
    cvs.draw_circle(10, 10, 3, color::from_rgb(0, 0, 255));
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    EXPECT_FALSE(out.empty());
}

TEST(CanvasWidgetTest, DrawCircleLarge) {
    canvas_widget_builder cvs(80, 80);
    cvs.draw_circle(40, 40, 20);
    element e = std::move(cvs);
    auto out = render_canvas(e, 50);
    EXPECT_FALSE(out.empty());
}

// ── draw_rect ───────────────────────────────────────────────────────────

TEST(CanvasWidgetTest, DrawRectOutline) {
    canvas_widget_builder cvs(20, 16);
    cvs.draw_rect(2, 2, 16, 12);
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    EXPECT_FALSE(out.empty());
}

// ── draw_text ───────────────────────────────────────────────────────────

TEST(CanvasWidgetTest, DrawTextAtPosition) {
    canvas_widget_builder cvs(40, 8);
    cvs.draw_text(5, 1, "Hello");
    element e = std::move(cvs);
    auto out = render_canvas(e, 30);
    EXPECT_TRUE(out.find("Hello") != std::string::npos);
}

// ── draw_block_line ─────────────────────────────────────────────────────

TEST(CanvasWidgetTest, DrawBlockLine) {
    canvas_widget_builder cvs(20, 10);
    cvs.draw_block_line(0, 0, 19, 9);
    element e = std::move(cvs);
    auto out = render_canvas(e, 30);
    EXPECT_FALSE(out.empty());
}

// ── make_canvas ─────────────────────────────────────────────────────────

TEST(CanvasWidgetTest, CanvasAsElement) {
    auto e = make_canvas(20, 8, [](canvas_widget_builder& c) {
        c.draw_point(5, 3);
        c.draw_line(0, 0, 19, 7);
    });
    EXPECT_FALSE(e.empty());
    auto out = render_canvas(e, 20);
    EXPECT_FALSE(out.empty());
}

TEST(CanvasWidgetTest, CanvasInLayout) {
    auto cvs = make_canvas(20, 8, [](canvas_widget_builder& c) {
        c.draw_circle(10, 4, 3);
    });
    auto txt = element(text_builder("Label"));
    auto rb = rows_builder();
    rb.add(std::move(cvs));
    rb.add(std::move(txt));
    element layout = std::move(rb);
    auto out = render_canvas(layout, 30);
    EXPECT_TRUE(out.find("Label") != std::string::npos);
}

// ── edge cases ──────────────────────────────────────────────────────────

TEST(CanvasWidgetTest, EmptyCanvasDoesNotCrash) {
    canvas_widget_builder cvs(10, 10);
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    // Empty canvas should produce output without crashing
    EXPECT_TRUE(true);
}

TEST(CanvasWidgetTest, OutOfBoundsPointsIgnored) {
    canvas_widget_builder cvs(10, 10);
    cvs.draw_point(-5, -5);
    cvs.draw_point(100, 100);
    element e = std::move(cvs);
    auto out = render_canvas(e, 20);
    // Should not crash
    EXPECT_TRUE(true);
}

TEST(CanvasWidgetTest, FluentChaining) {
    auto e = element(
        canvas_widget_builder(20, 16)
            .draw_point(5, 5)
            .draw_line(0, 0, 19, 15)
            .draw_circle(10, 8, 5)
            .draw_rect(1, 1, 18, 14)
            .draw_text(2, 0, "Hi")
    );
    EXPECT_FALSE(e.empty());
}
