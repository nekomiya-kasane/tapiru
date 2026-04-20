/**
 * @file kitty_keyboard_test.cpp
 * @brief Tests for Kitty keyboard protocol types and CSI u parsing.
 */

#include "tapiru/core/input.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── key_action enum ─────────────────────────────────────────────────────

TEST(KittyKeyboardTest, KeyActionValues) {
    EXPECT_EQ(static_cast<uint8_t>(key_action::press), 1u);
    EXPECT_EQ(static_cast<uint8_t>(key_action::repeat), 2u);
    EXPECT_EQ(static_cast<uint8_t>(key_action::release), 3u);
}

TEST(KittyKeyboardTest, KeyEventDefaultAction) {
    key_event ev;
    EXPECT_EQ(ev.action, key_action::press);
}

TEST(KittyKeyboardTest, KeyEventExplicitAction) {
    key_event ev{U'a', special_key::none, key_mod::none, key_action::release};
    EXPECT_EQ(ev.codepoint, U'a');
    EXPECT_EQ(ev.action, key_action::release);
}

// ── Extended key_mod flags (Kitty protocol) ─────────────────────────────

TEST(KittyKeyboardTest, SuperModifier) {
    auto mods = key_mod::super_;
    EXPECT_TRUE(has_mod(mods, key_mod::super_));
    EXPECT_FALSE(has_mod(mods, key_mod::ctrl));
}

TEST(KittyKeyboardTest, HyperModifier) {
    auto mods = key_mod::hyper;
    EXPECT_TRUE(has_mod(mods, key_mod::hyper));
    EXPECT_FALSE(has_mod(mods, key_mod::shift));
}

TEST(KittyKeyboardTest, CapsLockModifier) {
    auto mods = key_mod::caps_lock;
    EXPECT_TRUE(has_mod(mods, key_mod::caps_lock));
}

TEST(KittyKeyboardTest, NumLockModifier) {
    auto mods = key_mod::num_lock;
    EXPECT_TRUE(has_mod(mods, key_mod::num_lock));
}

TEST(KittyKeyboardTest, CombinedKittyModifiers) {
    auto mods = key_mod::ctrl | key_mod::super_ | key_mod::hyper;
    EXPECT_TRUE(has_mod(mods, key_mod::ctrl));
    EXPECT_TRUE(has_mod(mods, key_mod::super_));
    EXPECT_TRUE(has_mod(mods, key_mod::hyper));
    EXPECT_FALSE(has_mod(mods, key_mod::shift));
    EXPECT_FALSE(has_mod(mods, key_mod::alt));
    EXPECT_FALSE(has_mod(mods, key_mod::caps_lock));
    EXPECT_FALSE(has_mod(mods, key_mod::num_lock));
}

TEST(KittyKeyboardTest, AllModifiersCombined) {
    auto mods = key_mod::shift | key_mod::ctrl | key_mod::alt | key_mod::super_ | key_mod::hyper | key_mod::caps_lock |
                key_mod::num_lock;
    EXPECT_TRUE(has_mod(mods, key_mod::shift));
    EXPECT_TRUE(has_mod(mods, key_mod::ctrl));
    EXPECT_TRUE(has_mod(mods, key_mod::alt));
    EXPECT_TRUE(has_mod(mods, key_mod::super_));
    EXPECT_TRUE(has_mod(mods, key_mod::hyper));
    EXPECT_TRUE(has_mod(mods, key_mod::caps_lock));
    EXPECT_TRUE(has_mod(mods, key_mod::num_lock));
    EXPECT_EQ(static_cast<uint8_t>(mods), 127u);
}

// ── key_event variant in input_event ────────────────────────────────────

TEST(KittyKeyboardTest, InputEventWithAction) {
    key_event ke{U'x', special_key::none, key_mod::ctrl, key_action::repeat};
    input_event ev = ke;
    ASSERT_TRUE(std::holds_alternative<key_event>(ev));
    auto &got = std::get<key_event>(ev);
    EXPECT_EQ(got.codepoint, U'x');
    EXPECT_EQ(got.action, key_action::repeat);
    EXPECT_TRUE(has_mod(got.mods, key_mod::ctrl));
}

TEST(KittyKeyboardTest, SpecialKeyWithRelease) {
    key_event ke{0, special_key::escape, key_mod::none, key_action::release};
    EXPECT_EQ(ke.key, special_key::escape);
    EXPECT_EQ(ke.action, key_action::release);
    EXPECT_EQ(ke.codepoint, 0u);
}

TEST(KittyKeyboardTest, FunctionKeyWithModifiers) {
    key_event ke{0, special_key::f5, key_mod::shift | key_mod::super_, key_action::press};
    EXPECT_EQ(ke.key, special_key::f5);
    EXPECT_TRUE(has_mod(ke.mods, key_mod::shift));
    EXPECT_TRUE(has_mod(ke.mods, key_mod::super_));
    EXPECT_EQ(ke.action, key_action::press);
}
