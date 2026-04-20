/**
 * @file color_downgrade_test.cpp
 * @brief Tests for color::downgrade and double_underline attr.
 */

#include <gtest/gtest.h>

#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"

using namespace tapiru;

// ── color::downgrade ────────────────────────────────────────────────────

TEST(ColorDowngradeTest, DefaultColorUnchanged) {
    auto c = color::default_color();
    EXPECT_EQ(c.downgrade(0), color::default_color());
    EXPECT_EQ(c.downgrade(1), color::default_color());
    EXPECT_EQ(c.downgrade(2), color::default_color());
    EXPECT_EQ(c.downgrade(3), color::default_color());
}

TEST(ColorDowngradeTest, RgbTo256) {
    auto c = color::from_rgb(255, 0, 0);
    auto d = c.downgrade(2);  // palette_256
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngradeTest, RgbTo16) {
    auto c = color::from_rgb(255, 0, 0);
    auto d = c.downgrade(1);  // basic_16
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    // Should map to red (index 1 or 9)
    EXPECT_TRUE(d.r == 1 || d.r == 9);
}

TEST(ColorDowngradeTest, RgbToNone) {
    auto c = color::from_rgb(128, 64, 32);
    auto d = c.downgrade(0);
    EXPECT_TRUE(d.is_default());
}

TEST(ColorDowngradeTest, Idx256To16) {
    auto c = color::from_index_256(196);  // bright red in 256 palette
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
}

TEST(ColorDowngradeTest, Idx16Unchanged) {
    auto c = color::from_index_16(5);
    auto d = c.downgrade(1);
    EXPECT_EQ(d, c);
}

TEST(ColorDowngradeTest, NoDowngradeNeeded) {
    auto c = color::from_index_16(3);
    // Target is higher than current — no change
    auto d = c.downgrade(3);
    EXPECT_EQ(d, c);
}

TEST(ColorDowngradeTest, DowngradeChain) {
    auto c = color::from_rgb(0, 200, 0);
    auto d256 = c.downgrade(2);
    EXPECT_EQ(d256.kind, color_kind::indexed_256);
    auto d16 = d256.downgrade(1);
    EXPECT_EQ(d16.kind, color_kind::indexed_16);
    auto d0 = d16.downgrade(0);
    EXPECT_TRUE(d0.is_default());
}

TEST(ColorDowngradeTest, GrayscaleRamp) {
    // Pure gray should map to grayscale ramp in 256
    auto c = color::from_rgb(128, 128, 128);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngradeTest, WhiteRgbTo256) {
    auto c = color::from_rgb(255, 255, 255);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngradeTest, BlackRgbTo16) {
    auto c = color::from_rgb(0, 0, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_EQ(d.r, 0u);  // black
}

// ── double_underline attr ───────────────────────────────────────────────

TEST(DoubleUnderlineTest, AttrBitSet) {
    EXPECT_EQ(static_cast<uint16_t>(attr::double_underline), 256u);
}

TEST(DoubleUnderlineTest, CombineWithOtherAttrs) {
    auto a = attr::bold | attr::double_underline;
    EXPECT_TRUE(has_attr(a, attr::bold));
    EXPECT_TRUE(has_attr(a, attr::double_underline));
    EXPECT_FALSE(has_attr(a, attr::italic));
}

TEST(DoubleUnderlineTest, StyleWithDoubleUnderline) {
    style s;
    s.attrs = attr::double_underline;
    EXPECT_FALSE(s.is_default());
    EXPECT_TRUE(has_attr(s.attrs, attr::double_underline));
}
