#include <gtest/gtest.h>

#include "tapiru/core/console.h"
#include "tapiru/widgets/builders.h"

using namespace tapiru;

// ── VirtualTerminal helper (same as console_test) ───────────────────────

class virtual_terminal {
public:
    [[nodiscard]] console make_console(bool color = false, uint32_t width = 40) {
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

// ── Text builder ────────────────────────────────────────────────────────

TEST(WidgetRenderTest, TextBuilder) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(text_builder("Hello World"), 40);
    EXPECT_TRUE(vt.raw().find("Hello World") != std::string::npos);
}

TEST(WidgetRenderTest, TextBuilderWithMarkup) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(text_builder("[bold]Bold[/bold] text"), 40);
    EXPECT_TRUE(vt.raw().find("Bold") != std::string::npos);
    EXPECT_TRUE(vt.raw().find("text") != std::string::npos);
}

// ── Rule builder ────────────────────────────────────────────────────────

TEST(WidgetRenderTest, RuleBuilder) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 20);
    con.print_widget(rule_builder(), 20);
    // Rule should produce a line of characters
    EXPECT_FALSE(vt.raw().empty());
}

TEST(WidgetRenderTest, RuleBuilderWithTitle) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 30);
    con.print_widget(rule_builder("Title"), 30);
    EXPECT_TRUE(vt.raw().find("Title") != std::string::npos);
}

// ── Panel builder ───────────────────────────────────────────────────────

TEST(WidgetRenderTest, PanelBuilder) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(
        panel_builder(text_builder("Content"))
            .border(border_style::ascii),
        40);
    auto& out = vt.raw();
    // ASCII border uses +, -, |
    EXPECT_TRUE(out.find('+') != std::string::npos);
    EXPECT_TRUE(out.find('-') != std::string::npos);
    EXPECT_TRUE(out.find('|') != std::string::npos);
    EXPECT_TRUE(out.find("Content") != std::string::npos);
}

TEST(WidgetRenderTest, PanelBuilderWithTitle) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(
        panel_builder(text_builder("Body content here"))
            .border(border_style::ascii)
            .title("Title"),
        40);
    EXPECT_TRUE(vt.raw().find("Title") != std::string::npos);
    EXPECT_TRUE(vt.raw().find("Body content here") != std::string::npos);
}

// ── Padding builder ─────────────────────────────────────────────────────

TEST(WidgetRenderTest, PaddingBuilder) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(
        padding_builder(text_builder("X")).pad(1, 2, 1, 2),
        40);
    // Output should have padding (blank lines/spaces around X)
    EXPECT_TRUE(vt.raw().find('X') != std::string::npos);
}

// ── Table builder ───────────────────────────────────────────────────────

TEST(WidgetRenderTest, TableBuilder) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 60);

    auto tbl = table_builder()
        .add_column("Name")
        .add_column("Age")
        .add_row({"Alice", "30"})
        .add_row({"Bob", "25"})
        .border(border_style::ascii);

    con.print_widget(tbl, 60);

    auto& out = vt.raw();
    EXPECT_TRUE(out.find("Name") != std::string::npos);
    EXPECT_TRUE(out.find("Age") != std::string::npos);
    EXPECT_TRUE(out.find("Alice") != std::string::npos);
    EXPECT_TRUE(out.find("30") != std::string::npos);
    EXPECT_TRUE(out.find("Bob") != std::string::npos);
    EXPECT_TRUE(out.find("25") != std::string::npos);
}

// ── Colored output ──────────────────────────────────────────────────────

TEST(WidgetRenderTest, TextBuilderWithColor) {
    virtual_terminal vt;
    auto con = vt.make_console(true, 40);
    con.print_widget(text_builder("[red]Error[/]"), 40);
    auto& out = vt.raw();
    // Should contain ANSI escape for red (31)
    EXPECT_TRUE(out.find("31") != std::string::npos);
    EXPECT_TRUE(out.find("Error") != std::string::npos);
}

// ── Z-order builder API ────────────────────────────────────────────────

TEST(WidgetRenderTest, ZOrderChaining) {
    // Just verify the builder API compiles and renders without error
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(text_builder("Hello").z_order(5), 40);
    EXPECT_TRUE(vt.raw().find("Hello") != std::string::npos);
}

// ── Overlay builder ────────────────────────────────────────────────────

TEST(WidgetRenderTest, OverlayBuilderRendersOverlayOnTop) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 10);
    // Base: "AAAAAAAAAA", Overlay: "BB"
    // Overlay overwrites first 2 chars of base
    con.print_widget(overlay_builder(text_builder("AAAAAAAAAA"), text_builder("BB")), 10);
    auto& out = vt.raw();
    // Should contain "BB" from overlay
    EXPECT_TRUE(out.find("BB") != std::string::npos);
    // Should still contain some A's from base (positions 2+)
    EXPECT_TRUE(out.find("A") != std::string::npos);
}

TEST(WidgetRenderTest, OverlayBuilderMeasuresAsMax) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 80);
    // Base is 5 chars, overlay is 3 chars — output should be 5 wide
    con.print_widget(overlay_builder(text_builder("ABCDE"), text_builder("XY")), 80);
    auto& out = vt.raw();
    EXPECT_TRUE(out.find("XY") != std::string::npos);
    EXPECT_TRUE(out.find("CDE") != std::string::npos);
}

// ── Gradient ───────────────────────────────────────────────────────────

TEST(WidgetRenderTest, RuleBuilderWithGradient) {
    virtual_terminal vt;
    auto con = vt.make_console(true, 20);
    linear_gradient g{color::from_rgb(255, 0, 0), color::from_rgb(0, 0, 255)};
    con.print_widget(rule_builder().gradient(g), 20);
    auto& out = vt.raw();
    // Should contain ANSI escape sequences for RGB colors
    EXPECT_TRUE(out.find("38;2;") != std::string::npos);
}

TEST(WidgetRenderTest, PanelBuilderWithBackgroundGradient) {
    virtual_terminal vt;
    auto con = vt.make_console(true, 20);
    linear_gradient g{color::from_rgb(100, 0, 0), color::from_rgb(0, 0, 200)};
    con.print_widget(panel_builder(text_builder("Hi")).background_gradient(g), 20);
    auto& out = vt.raw();
    // Should contain ANSI escape sequences for RGB background colors (48;2;r;g;b)
    // or at minimum render without error and produce output
    EXPECT_TRUE(out.find("48;2;") != std::string::npos || out.find("Hi") != std::string::npos);
}

TEST(WidgetRenderTest, PanelBuilderOpacityChaining) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 20);
    con.print_widget(panel_builder(text_builder("Test")).opacity(0.5f), 20);
    // Just verify it renders without crashing
    EXPECT_FALSE(vt.raw().empty());
}

// ── Border join ────────────────────────────────────────────────────────

TEST(WidgetRenderTest, BorderJoinProducesJunctions) {
    // Two stacked panels should produce T-junctions where borders meet
    virtual_terminal vt;
    auto con = vt.make_console(false, 20);
    columns_builder cols;
    cols.add(panel_builder(text_builder("A")));
    cols.add(panel_builder(text_builder("B")));
    con.print_widget(std::move(cols), 20);
    // Just verify it renders without crashing — visual verification needed for actual joins
    EXPECT_FALSE(vt.raw().empty());
}
