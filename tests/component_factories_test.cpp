/**
 * @file component_factories_test.cpp
 * @brief Tests for interactive widget component factories.
 */

#include "tapiru/core/console.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/component_factories.h"

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

static std::string render_comp(component c, uint32_t width = 40) {
    virtual_terminal vt;
    auto con = vt.make_console();
    auto elem = c->render();
    con.print_widget(elem, width);
    return vt.raw();
}

// ── button ──────────────────────────────────────────────────────────────

TEST(ComponentFactoryTest, ButtonRendersLabel) {
    auto btn = make_button({.label = "Click Me"});
    auto out = render_comp(btn);
    EXPECT_TRUE(out.find("Click Me") != std::string::npos);
}

TEST(ComponentFactoryTest, ButtonIsFocusable) {
    auto btn = make_button({.label = "OK"});
    EXPECT_TRUE(btn->focusable());
}

TEST(ComponentFactoryTest, ButtonOnClickFires) {
    bool clicked = false;
    auto btn = make_button({.label = "Go", .on_click = [&] { clicked = true; }});
    btn->set_focused(true);
    key_event enter{0, special_key::enter, key_mod::none};
    EXPECT_TRUE(btn->on_event(enter));
    EXPECT_TRUE(clicked);
}

TEST(ComponentFactoryTest, ButtonIgnoresEnterWhenNotFocused) {
    bool clicked = false;
    auto btn = make_button({.label = "Go", .on_click = [&] { clicked = true; }});
    key_event enter{0, special_key::enter, key_mod::none};
    EXPECT_FALSE(btn->on_event(enter));
    EXPECT_FALSE(clicked);
}

// ── checkbox ────────────────────────────────────────────────────────────

TEST(ComponentFactoryTest, CheckboxRendersLabel) {
    bool val = false;
    auto cb = make_checkbox({.label = "Enable", .value = &val});
    auto out = render_comp(cb);
    EXPECT_TRUE(out.find("Enable") != std::string::npos);
}

TEST(ComponentFactoryTest, CheckboxTogglesOnEnter) {
    bool val = false;
    auto cb = make_checkbox({.label = "Toggle", .value = &val});
    cb->set_focused(true);
    key_event enter{0, special_key::enter, key_mod::none};
    EXPECT_TRUE(cb->on_event(enter));
    EXPECT_TRUE(val);
    EXPECT_TRUE(cb->on_event(enter));
    EXPECT_FALSE(val);
}

TEST(ComponentFactoryTest, CheckboxTogglesOnSpace) {
    bool val = true;
    auto cb = make_checkbox({.label = "X", .value = &val});
    cb->set_focused(true);
    key_event space{U' ', special_key::none, key_mod::none};
    EXPECT_TRUE(cb->on_event(space));
    EXPECT_FALSE(val);
}

// ── radio_group ─────────────────────────────────────────────────────────

TEST(ComponentFactoryTest, RadioGroupRendersOptions) {
    int sel = 0;
    auto rg = make_radio_group({.options = {"A", "B", "C"}, .selected = &sel});
    auto out = render_comp(rg);
    EXPECT_TRUE(out.find("A") != std::string::npos);
    EXPECT_TRUE(out.find("B") != std::string::npos);
    EXPECT_TRUE(out.find("C") != std::string::npos);
}

TEST(ComponentFactoryTest, RadioGroupNavigates) {
    int sel = 0;
    auto rg = make_radio_group({.options = {"A", "B", "C"}, .selected = &sel});
    key_event down{0, special_key::down, key_mod::none};
    EXPECT_TRUE(rg->on_event(down));
    EXPECT_EQ(sel, 1);
    EXPECT_TRUE(rg->on_event(down));
    EXPECT_EQ(sel, 2);
    EXPECT_TRUE(rg->on_event(down));
    EXPECT_EQ(sel, 0); // wraps
}

TEST(ComponentFactoryTest, RadioGroupNavigatesUp) {
    int sel = 0;
    auto rg = make_radio_group({.options = {"A", "B"}, .selected = &sel});
    key_event up{0, special_key::up, key_mod::none};
    EXPECT_TRUE(rg->on_event(up));
    EXPECT_EQ(sel, 1); // wraps to last
}

// ── selectable_list ─────────────────────────────────────────────────────

TEST(ComponentFactoryTest, SelectableListRendersItems) {
    int cursor = 0;
    auto sl = make_selectable_list({.items = {"Item1", "Item2"}, .cursor = &cursor});
    auto out = render_comp(sl);
    EXPECT_TRUE(out.find("Item1") != std::string::npos);
    EXPECT_TRUE(out.find("Item2") != std::string::npos);
}

TEST(ComponentFactoryTest, SelectableListNavigates) {
    int cursor = 0;
    auto sl = make_selectable_list({.items = {"A", "B", "C"}, .cursor = &cursor});
    key_event down{0, special_key::down, key_mod::none};
    EXPECT_TRUE(sl->on_event(down));
    EXPECT_EQ(cursor, 1);
}

TEST(ComponentFactoryTest, SelectableListOnSelect) {
    int cursor = 0;
    int selected = -1;
    auto sl =
        make_selectable_list({.items = {"X", "Y"}, .cursor = &cursor, .on_select = [&](int idx) { selected = idx; }});
    key_event enter{0, special_key::enter, key_mod::none};
    EXPECT_TRUE(sl->on_event(enter));
    EXPECT_EQ(selected, 0);
}

// ── text_input ──────────────────────────────────────────────────────────

TEST(ComponentFactoryTest, TextInputRendersPlaceholder) {
    std::string buf;
    auto ti = make_text_input({.buffer = &buf, .placeholder = "Type here"});
    auto out = render_comp(ti);
    EXPECT_TRUE(out.find("Type here") != std::string::npos);
}

TEST(ComponentFactoryTest, TextInputAcceptsCharacters) {
    std::string buf;
    auto ti = make_text_input({.buffer = &buf});
    ti->set_focused(true);
    key_event a{U'a', special_key::none, key_mod::none};
    key_event b{U'b', special_key::none, key_mod::none};
    EXPECT_TRUE(ti->on_event(a));
    EXPECT_TRUE(ti->on_event(b));
    EXPECT_EQ(buf, "ab");
}

TEST(ComponentFactoryTest, TextInputBackspace) {
    std::string buf = "abc";
    auto ti = make_text_input({.buffer = &buf});
    ti->set_focused(true);
    // Move cursor to end by typing (cursor starts at 0, so we need to go to end)
    key_event end_key{0, special_key::end, key_mod::none};
    ti->on_event(end_key);
    key_event bs{0, special_key::backspace, key_mod::none};
    EXPECT_TRUE(ti->on_event(bs));
    EXPECT_EQ(buf, "ab");
}

TEST(ComponentFactoryTest, TextInputOnChange) {
    std::string buf;
    std::string last_change;
    auto ti = make_text_input({.buffer = &buf, .on_change = [&](const std::string &s) { last_change = s; }});
    ti->set_focused(true);
    key_event x{U'x', special_key::none, key_mod::none};
    ti->on_event(x);
    EXPECT_EQ(last_change, "x");
}

// ── slider ──────────────────────────────────────────────────────────────

TEST(ComponentFactoryTest, SliderRendersValue) {
    float val = 0.5f;
    auto sl = make_slider({.value = &val, .min_val = 0.f, .max_val = 1.f});
    auto out = render_comp(sl);
    EXPECT_FALSE(out.empty());
}

TEST(ComponentFactoryTest, SliderIncrementsOnRight) {
    float val = 0.5f;
    auto sl = make_slider({.value = &val, .min_val = 0.f, .max_val = 1.f, .step = 0.1f});
    sl->set_focused(true);
    key_event right{0, special_key::right, key_mod::none};
    EXPECT_TRUE(sl->on_event(right));
    EXPECT_NEAR(val, 0.6f, 0.001f);
}

TEST(ComponentFactoryTest, SliderDecrementsOnLeft) {
    float val = 0.5f;
    auto sl = make_slider({.value = &val, .min_val = 0.f, .max_val = 1.f, .step = 0.1f});
    sl->set_focused(true);
    key_event left{0, special_key::left, key_mod::none};
    EXPECT_TRUE(sl->on_event(left));
    EXPECT_NEAR(val, 0.4f, 0.001f);
}

TEST(ComponentFactoryTest, SliderClampsToMax) {
    float val = 0.95f;
    auto sl = make_slider({.value = &val, .min_val = 0.f, .max_val = 1.f, .step = 0.1f});
    sl->set_focused(true);
    key_event right{0, special_key::right, key_mod::none};
    sl->on_event(right);
    EXPECT_LE(val, 1.0f);
}

TEST(ComponentFactoryTest, SliderClampsToMin) {
    float val = 0.05f;
    auto sl = make_slider({.value = &val, .min_val = 0.f, .max_val = 1.f, .step = 0.1f});
    sl->set_focused(true);
    key_event left{0, special_key::left, key_mod::none};
    sl->on_event(left);
    EXPECT_GE(val, 0.0f);
}
