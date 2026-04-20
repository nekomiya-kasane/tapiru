/**
 * @file input_test.cpp
 * @brief Tests for input event types, event_table, focus_manager, and hit_test.
 */

#include <gtest/gtest.h>
#include "tapiru/core/input.h"

using namespace tapiru;

// ── Event types ────────────────────────────────────────────────────────

TEST(InputTest, KeyEventDefaults) {
    key_event ev;
    EXPECT_EQ(ev.codepoint, 0u);
    EXPECT_EQ(ev.key, special_key::none);
    EXPECT_EQ(ev.mods, key_mod::none);
}

TEST(InputTest, MouseEventDefaults) {
    mouse_event ev;
    EXPECT_EQ(ev.x, 0u);
    EXPECT_EQ(ev.y, 0u);
    EXPECT_EQ(ev.button, mouse_button::none);
    EXPECT_EQ(ev.action, mouse_action::press);
}

TEST(InputTest, ResizeEventDefaults) {
    resize_event ev;
    EXPECT_EQ(ev.width, 0u);
    EXPECT_EQ(ev.height, 0u);
}

TEST(InputTest, KeyModBitOps) {
    auto mods = key_mod::shift | key_mod::ctrl;
    EXPECT_TRUE(has_mod(mods, key_mod::shift));
    EXPECT_TRUE(has_mod(mods, key_mod::ctrl));
    EXPECT_FALSE(has_mod(mods, key_mod::alt));
}

TEST(InputTest, InputEventVariant) {
    input_event ev = key_event{U'a', special_key::none, key_mod::none};
    EXPECT_TRUE(std::holds_alternative<key_event>(ev));

    ev = mouse_event{10, 20, mouse_button::left, mouse_action::press, key_mod::none};
    EXPECT_TRUE(std::holds_alternative<mouse_event>(ev));

    ev = resize_event{80, 24};
    EXPECT_TRUE(std::holds_alternative<resize_event>(ev));
}

// ── event_table ────────────────────────────────────────────────────────

TEST(EventTableTest, DispatchKeyToRegisteredNode) {
    event_table et;
    bool called = false;
    et.on_key(42, [&](const key_event& ev) {
        called = true;
        EXPECT_EQ(ev.codepoint, U'x');
        return true;
    });

    key_event ev{U'x'};
    EXPECT_TRUE(et.dispatch_key(42, ev));
    EXPECT_TRUE(called);
}

TEST(EventTableTest, DispatchKeyToUnregisteredNodeReturnsFalse) {
    event_table et;
    key_event ev{U'x'};
    EXPECT_FALSE(et.dispatch_key(99, ev));
}

TEST(EventTableTest, DispatchMouseToRegisteredNode) {
    event_table et;
    bool called = false;
    et.on_mouse(7, [&](const mouse_event& ev) {
        called = true;
        EXPECT_EQ(ev.x, 5u);
        EXPECT_EQ(ev.y, 10u);
        return true;
    });

    mouse_event ev{5, 10, mouse_button::left, mouse_action::press};
    EXPECT_TRUE(et.dispatch_mouse(7, ev));
    EXPECT_TRUE(called);
}

TEST(EventTableTest, ClearRemovesAllHandlers) {
    event_table et;
    et.on_key(1, [](const key_event&) { return true; });
    et.clear();
    EXPECT_FALSE(et.dispatch_key(1, key_event{}));
}

TEST(EventTableTest, OverwriteHandler) {
    event_table et;
    int call_count = 0;
    et.on_key(1, [&](const key_event&) { call_count = 1; return true; });
    et.on_key(1, [&](const key_event&) { call_count = 2; return true; });
    et.dispatch_key(1, key_event{});
    EXPECT_EQ(call_count, 2);
}

// ── focus_manager ──────────────────────────────────────────────────────

TEST(FocusManagerTest, EmptyFocusable) {
    focus_manager fm;
    EXPECT_EQ(fm.focused(), UINT32_MAX);
    fm.focus_next();
    EXPECT_EQ(fm.focused(), UINT32_MAX);
}

TEST(FocusManagerTest, SetFocusableAutoFocusesFirst) {
    focus_manager fm;
    fm.set_focusable_nodes({10, 20, 30});
    EXPECT_EQ(fm.focused(), 10u);
}

TEST(FocusManagerTest, FocusNext) {
    focus_manager fm;
    fm.set_focusable_nodes({10, 20, 30});
    fm.focus_next();
    EXPECT_EQ(fm.focused(), 20u);
    fm.focus_next();
    EXPECT_EQ(fm.focused(), 30u);
    fm.focus_next();  // wraps
    EXPECT_EQ(fm.focused(), 10u);
}

TEST(FocusManagerTest, FocusPrev) {
    focus_manager fm;
    fm.set_focusable_nodes({10, 20, 30});
    fm.focus_prev();  // wraps from first to last
    EXPECT_EQ(fm.focused(), 30u);
    fm.focus_prev();
    EXPECT_EQ(fm.focused(), 20u);
}

TEST(FocusManagerTest, FocusSpecificNode) {
    focus_manager fm;
    fm.set_focusable_nodes({10, 20, 30});
    fm.focus(20);
    EXPECT_EQ(fm.focused(), 20u);
    EXPECT_TRUE(fm.has_focus(20));
    EXPECT_FALSE(fm.has_focus(10));
}

TEST(FocusManagerTest, FocusInvalidNodeIgnored) {
    focus_manager fm;
    fm.set_focusable_nodes({10, 20, 30});
    fm.focus(99);  // not in list
    EXPECT_EQ(fm.focused(), 10u);  // unchanged
}

TEST(FocusManagerTest, SetFocusablePreservesCurrentFocus) {
    focus_manager fm;
    fm.set_focusable_nodes({10, 20, 30});
    fm.focus(20);
    fm.set_focusable_nodes({10, 20, 30, 40});
    EXPECT_EQ(fm.focused(), 20u);  // preserved
}

TEST(FocusManagerTest, SetFocusableResetsIfCurrentRemoved) {
    focus_manager fm;
    fm.set_focusable_nodes({10, 20, 30});
    fm.focus(20);
    fm.set_focusable_nodes({10, 30});  // 20 removed
    EXPECT_EQ(fm.focused(), 10u);  // reset to first
}
