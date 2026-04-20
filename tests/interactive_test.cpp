/**
 * @file interactive_test.cpp
 * @brief Tests for interactive widget builders.
 */

#include <gtest/gtest.h>
#include "tapiru/widgets/interactive.h"
#include "tapiru/core/console.h"

using namespace tapiru;

// ── VirtualTerminal helper ──────────────────────────────────────────────

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

// Helper: render widget to string via virtual_terminal
static std::string render(auto&& builder, uint32_t width = 40) {
    virtual_terminal vt;
    auto con = vt.make_console(false, width);
    con.print_widget(std::forward<decltype(builder)>(builder), width);
    return vt.raw();
}

// ── button_builder ─────────────────────────────────────────────────────

TEST(InteractiveTest, ButtonRendersLabel) {
    auto out = render(button_builder("Submit"));
    EXPECT_TRUE(out.find("Submit") != std::string::npos);
}

TEST(InteractiveTest, ButtonFocusedRendersLabel) {
    auto out = render(button_builder("OK").focused(true));
    EXPECT_TRUE(out.find("OK") != std::string::npos);
}

TEST(InteractiveTest, ButtonClickHandler) {
    bool clicked = false;
    auto btn = button_builder("Go").on_click([&] { clicked = true; });
    btn.click_handler()();
    EXPECT_TRUE(clicked);
}

// ── checkbox_builder ───────────────────────────────────────────────────

TEST(InteractiveTest, CheckboxUnchecked) {
    bool val = false;
    auto out = render(checkbox_builder("Enable", &val));
    EXPECT_TRUE(out.find("Enable") != std::string::npos);
}

TEST(InteractiveTest, CheckboxChecked) {
    bool val = true;
    auto out = render(checkbox_builder("Enable", &val));
    EXPECT_TRUE(out.find("Enable") != std::string::npos);
}

// ── radio_group_builder ────────────────────────────────────────────────

TEST(InteractiveTest, RadioGroupRendersOptions) {
    int sel = 1;
    auto out = render(radio_group_builder({"A", "B", "C"}, &sel));
    EXPECT_TRUE(out.find("A") != std::string::npos);
    EXPECT_TRUE(out.find("B") != std::string::npos);
    EXPECT_TRUE(out.find("C") != std::string::npos);
}

// ── selectable_list_builder ────────────────────────────────────────────

TEST(InteractiveTest, SelectableListRendersItems) {
    int cursor = 0;
    auto out = render(selectable_list_builder({"Item1", "Item2", "Item3"}, &cursor));
    EXPECT_TRUE(out.find("Item1") != std::string::npos);
    EXPECT_TRUE(out.find("Item2") != std::string::npos);
}

TEST(InteractiveTest, SelectableListWithVisibleCount) {
    int cursor = 0;
    auto out = render(selectable_list_builder({"A", "B", "C", "D", "E"}, &cursor).visible_count(3));
    EXPECT_FALSE(out.empty());
}

// ── text_input_builder ─────────────────────────────────────────────────

TEST(InteractiveTest, TextInputRendersBuffer) {
    std::string buf = "hello";
    auto out = render(text_input_builder(&buf).width(20));
    EXPECT_TRUE(out.find("hello") != std::string::npos);
}

TEST(InteractiveTest, TextInputPlaceholder) {
    std::string buf;
    auto out = render(text_input_builder(&buf).placeholder("Type here...").width(20));
    EXPECT_TRUE(out.find("Type here") != std::string::npos);
}

// ── slider_builder ─────────────────────────────────────────────────────

TEST(InteractiveTest, SliderRendersBar) {
    float val = 0.5f;
    auto out = render(slider_builder(&val, 0.0f, 1.0f).width(20));
    EXPECT_FALSE(out.empty());
    EXPECT_TRUE(out.find("%") != std::string::npos);
}

TEST(InteractiveTest, SliderZeroPercent) {
    float val = 0.0f;
    auto out = render(slider_builder(&val, 0.0f, 100.0f).width(20));
    EXPECT_TRUE(out.find("0%") != std::string::npos);
}

TEST(InteractiveTest, SliderHundredPercent) {
    float val = 100.0f;
    auto out = render(slider_builder(&val, 0.0f, 100.0f).width(20));
    EXPECT_TRUE(out.find("100%") != std::string::npos);
}
