#include "tapiru/layout/types.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── Rect ────────────────────────────────────────────────────────────────

TEST(RectTest, DefaultIsZero) {
    rect r;
    EXPECT_EQ(r.x, 0u);
    EXPECT_EQ(r.y, 0u);
    EXPECT_EQ(r.w, 0u);
    EXPECT_EQ(r.h, 0u);
    EXPECT_TRUE(r.empty());
}

TEST(RectTest, NonEmpty) {
    rect r{0, 0, 10, 5};
    EXPECT_FALSE(r.empty());
}

TEST(RectTest, Equality) {
    rect a{1, 2, 3, 4};
    rect b{1, 2, 3, 4};
    rect c{1, 2, 3, 5};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(RectTest, Inset) {
    rect r{0, 0, 20, 10};
    auto inner = r.inset(1, 2, 1, 2);
    EXPECT_EQ(inner.x, 2u);
    EXPECT_EQ(inner.y, 1u);
    EXPECT_EQ(inner.w, 16u);
    EXPECT_EQ(inner.h, 8u);
}

TEST(RectTest, InsetClamps) {
    rect r{0, 0, 4, 2};
    auto inner = r.inset(5, 5, 5, 5);
    EXPECT_EQ(inner.w, 0u);
    EXPECT_EQ(inner.h, 0u);
}

// ── BoxConstraints ──────────────────────────────────────────────────────

TEST(BoxConstraintsTest, DefaultIsUnbounded) {
    box_constraints bc;
    EXPECT_EQ(bc.min_w, 0u);
    EXPECT_EQ(bc.max_w, unbounded);
    EXPECT_EQ(bc.min_h, 0u);
    EXPECT_EQ(bc.max_h, unbounded);
}

TEST(BoxConstraintsTest, Tight) {
    auto bc = box_constraints::tight(80, 24);
    EXPECT_EQ(bc.min_w, 80u);
    EXPECT_EQ(bc.max_w, 80u);
    EXPECT_EQ(bc.min_h, 24u);
    EXPECT_EQ(bc.max_h, 24u);
}

TEST(BoxConstraintsTest, Loose) {
    auto bc = box_constraints::loose(100, 50);
    EXPECT_EQ(bc.min_w, 0u);
    EXPECT_EQ(bc.max_w, 100u);
    EXPECT_EQ(bc.min_h, 0u);
    EXPECT_EQ(bc.max_h, 50u);
}

TEST(BoxConstraintsTest, ConstrainClampsUp) {
    auto bc = box_constraints{10, 80, 5, 24};
    auto m = bc.constrain({3, 2});
    EXPECT_EQ(m.width, 10u);
    EXPECT_EQ(m.height, 5u);
}

TEST(BoxConstraintsTest, ConstrainClampsDown) {
    auto bc = box_constraints{10, 80, 5, 24};
    auto m = bc.constrain({200, 100});
    EXPECT_EQ(m.width, 80u);
    EXPECT_EQ(m.height, 24u);
}

TEST(BoxConstraintsTest, ConstrainPassthrough) {
    auto bc = box_constraints{10, 80, 5, 24};
    auto m = bc.constrain({40, 12});
    EXPECT_EQ(m.width, 40u);
    EXPECT_EQ(m.height, 12u);
}

TEST(BoxConstraintsTest, Equality) {
    auto a = box_constraints::tight(80, 24);
    auto b = box_constraints::tight(80, 24);
    auto c = box_constraints::tight(80, 25);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ── Measurement ─────────────────────────────────────────────────────────

TEST(MeasurementTest, Default) {
    measurement m;
    EXPECT_EQ(m.width, 0u);
    EXPECT_EQ(m.height, 0u);
}

// ── Border chars ────────────────────────────────────────────────────────

TEST(BorderCharsTest, Ascii) {
    auto bc = get_border_chars(border_style::ascii);
    EXPECT_EQ(bc.top_left, U'+');
    EXPECT_EQ(bc.horizontal, U'-');
    EXPECT_EQ(bc.vertical, U'|');
}

TEST(BorderCharsTest, Rounded) {
    auto bc = get_border_chars(border_style::rounded);
    EXPECT_EQ(bc.top_left, U'\x256D');
    EXPECT_EQ(bc.horizontal, U'\x2500');
    EXPECT_EQ(bc.vertical, U'\x2502');
}

TEST(BorderCharsTest, Heavy) {
    auto bc = get_border_chars(border_style::heavy);
    EXPECT_EQ(bc.top_left, U'\x250F');
    EXPECT_EQ(bc.horizontal, U'\x2501');
}

TEST(BorderCharsTest, Double) {
    auto bc = get_border_chars(border_style::double_);
    EXPECT_EQ(bc.top_left, U'\x2554');
    EXPECT_EQ(bc.horizontal, U'\x2550');
}

TEST(BorderCharsTest, None) {
    auto bc = get_border_chars(border_style::none);
    EXPECT_EQ(bc.top_left, U' ');
    EXPECT_EQ(bc.horizontal, U' ');
}
