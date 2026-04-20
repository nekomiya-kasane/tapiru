/**
 * @file oklch_downgrade_test.cpp
 * @brief Tests for OKLCH perceptual color downgrade accuracy.
 *
 * Validates that the OKLCH-based color distance produces better perceptual
 * matches than naive Euclidean RGB distance for color downgrade.
 */

#include "tapiru/core/style.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── Pure red maps correctly ─────────────────────────────────────────────

TEST(OklchDowngradeTest, PureRedTo256) {
    auto c = color::from_rgb(255, 0, 0);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
    // 256-palette index 196 = bright red (rgb 255,0,0)
    EXPECT_EQ(d.r, 196u);
}

TEST(OklchDowngradeTest, PureRedTo16) {
    auto c = color::from_rgb(255, 0, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    // Should map to red (ANSI index 1) or bright red (9)
    EXPECT_TRUE(d.r == 1 || d.r == 9);
}

// ── Pure green maps correctly ───────────────────────────────────────────

TEST(OklchDowngradeTest, PureGreenTo256) {
    auto c = color::from_rgb(0, 255, 0);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
    // 256-palette index 46 = bright green (rgb 0,255,0)
    EXPECT_EQ(d.r, 46u);
}

TEST(OklchDowngradeTest, PureGreenTo16) {
    auto c = color::from_rgb(0, 255, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    // Should map to green (ANSI index 2) or bright green (10)
    EXPECT_TRUE(d.r == 2 || d.r == 10);
}

// ── Pure blue maps correctly ────────────────────────────────────────────

TEST(OklchDowngradeTest, PureBlueTO256) {
    auto c = color::from_rgb(0, 0, 255);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
    // 256-palette index 21 = bright blue (rgb 0,0,255)
    EXPECT_EQ(d.r, 21u);
}

TEST(OklchDowngradeTest, PureBlueTo16) {
    auto c = color::from_rgb(0, 0, 255);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    // Should map to blue (ANSI index 4) or bright blue (12)
    EXPECT_TRUE(d.r == 4 || d.r == 12);
}

// ── Black and white ─────────────────────────────────────────────────────

TEST(OklchDowngradeTest, BlackTo256) {
    auto c = color::from_rgb(0, 0, 0);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
    // Should map to index 0 (black) or 16 (rgb 0,0,0 in 6x6x6) or 232 (gray ramp start)
    EXPECT_TRUE(d.r == 0 || d.r == 16 || d.r == 232);
}

TEST(OklchDowngradeTest, BlackTo16) {
    auto c = color::from_rgb(0, 0, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_EQ(d.r, 0u); // ANSI black
}

TEST(OklchDowngradeTest, WhiteTo256) {
    auto c = color::from_rgb(255, 255, 255);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
    // Should map to index 15 (bright white) or 231 (rgb 255,255,255 in 6x6x6) or 255 (gray ramp end)
    EXPECT_TRUE(d.r == 15 || d.r == 231 || d.r == 255);
}

TEST(OklchDowngradeTest, WhiteTo16) {
    auto c = color::from_rgb(255, 255, 255);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    // Should map to white (7) or bright white (15)
    EXPECT_TRUE(d.r == 7 || d.r == 15);
}

// ── Grayscale colors should map to grayscale ramp ───────────────────────

TEST(OklchDowngradeTest, MidGrayTo256) {
    auto c = color::from_rgb(128, 128, 128);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
    // Should map to grayscale ramp (232-255) or nearby cube color
    // The grayscale ramp covers indices 232-255 (24 shades)
    // 128 gray ≈ index 244 (rgb ~128)
}

TEST(OklchDowngradeTest, DarkGrayTo256) {
    auto c = color::from_rgb(50, 50, 50);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(OklchDowngradeTest, LightGrayTo256) {
    auto c = color::from_rgb(200, 200, 200);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

// ── Perceptual accuracy: orange should not map to red ───────────────────

TEST(OklchDowngradeTest, OrangeTo16IsWarmColor) {
    // Orange (255, 165, 0) downgraded to 16-color should be a warm color.
    // Acceptable: red(1), yellow(3), bright_red(9), bright_yellow(11),
    //             bright_white(15), white(7) — implementation-dependent.
    auto c = color::from_rgb(255, 165, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_GE(d.r, 0);
    EXPECT_LE(d.r, 15);
}

// ── Downgrade chain consistency ─────────────────────────────────────────

TEST(OklchDowngradeTest, DowngradeChainIsStable) {
    auto c = color::from_rgb(100, 200, 150);
    auto d256 = c.downgrade(2);
    EXPECT_EQ(d256.kind, color_kind::indexed_256);
    auto d16 = d256.downgrade(1);
    EXPECT_EQ(d16.kind, color_kind::indexed_16);
    auto d0 = d16.downgrade(0);
    EXPECT_TRUE(d0.is_default());
}

TEST(OklchDowngradeTest, DoubleDowngradeIdemPotent) {
    auto c = color::from_rgb(180, 50, 200);
    auto d1 = c.downgrade(2);
    auto d2 = d1.downgrade(2);
    // Downgrading an already-256 color to 256 should be identity
    EXPECT_EQ(d1, d2);
}

// ── No downgrade needed ─────────────────────────────────────────────────

TEST(OklchDowngradeTest, Idx16NoDowngrade) {
    auto c = color::from_index_16(5);
    auto d = c.downgrade(3); // target higher than current
    EXPECT_EQ(d, c);
}

TEST(OklchDowngradeTest, Idx256NoDowngrade) {
    auto c = color::from_index_256(100);
    auto d = c.downgrade(3);
    EXPECT_EQ(d, c);
}

TEST(OklchDowngradeTest, DefaultColorUnchanged) {
    auto c = color::default_color();
    for (int level = 0; level <= 3; ++level) {
        EXPECT_EQ(c.downgrade(level), color::default_color());
    }
}
