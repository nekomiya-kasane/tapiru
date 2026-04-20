#include <gtest/gtest.h>

#include "tapiru/core/style.h"
#include "tapiru/core/style_table.h"

using namespace tapiru;

// ── Color tests ─────────────────────────────────────────────────────────

TEST(ColorTest, DefaultColor) {
    auto c = color::default_color();
    EXPECT_TRUE(c.is_default());
    EXPECT_EQ(c.kind, color_kind::default_color);
}

TEST(ColorTest, Index16) {
    auto c = color::from_index_16(5);
    EXPECT_FALSE(c.is_default());
    EXPECT_EQ(c.kind, color_kind::indexed_16);
    EXPECT_EQ(c.r, 5);
}

TEST(ColorTest, Index256) {
    auto c = color::from_index_256(200);
    EXPECT_EQ(c.kind, color_kind::indexed_256);
    EXPECT_EQ(c.r, 200);
}

TEST(ColorTest, Rgb) {
    auto c = color::from_rgb(10, 20, 30);
    EXPECT_EQ(c.kind, color_kind::rgb);
    EXPECT_EQ(c.r, 10);
    EXPECT_EQ(c.g, 20);
    EXPECT_EQ(c.b, 30);
}

TEST(ColorTest, Equality) {
    EXPECT_EQ(color::from_rgb(1, 2, 3), color::from_rgb(1, 2, 3));
    EXPECT_NE(color::from_rgb(1, 2, 3), color::from_rgb(1, 2, 4));
    EXPECT_NE(color::default_color(), color::from_index_16(0));
}

TEST(ColorTest, SizeIs4Bytes) {
    static_assert(sizeof(color) == 4);
}

TEST(ColorTest, NamedColors) {
    EXPECT_EQ(colors::red.kind, color_kind::indexed_16);
    EXPECT_EQ(colors::red.r, 1);
    EXPECT_EQ(colors::bright_cyan.r, 14);
}

// ── Attr tests ──────────────────────────────────────────────────────────

TEST(AttrTest, BitwiseOr) {
    auto a = attr::bold | attr::italic;
    EXPECT_TRUE(has_attr(a, attr::bold));
    EXPECT_TRUE(has_attr(a, attr::italic));
    EXPECT_FALSE(has_attr(a, attr::underline));
}

TEST(AttrTest, BitwiseAnd) {
    auto a = attr::bold | attr::italic | attr::strike;
    auto masked = a & attr::bold;
    EXPECT_TRUE(has_attr(masked, attr::bold));
    EXPECT_FALSE(has_attr(masked, attr::italic));
}

TEST(AttrTest, BitwiseNot) {
    auto a = attr::bold | attr::italic;
    auto cleared = a & ~attr::bold;
    EXPECT_FALSE(has_attr(cleared, attr::bold));
    EXPECT_TRUE(has_attr(cleared, attr::italic));
}

TEST(AttrTest, CompoundAssign) {
    auto a = attr::none;
    a |= attr::underline;
    EXPECT_TRUE(has_attr(a, attr::underline));
    a &= ~attr::underline;
    EXPECT_FALSE(has_attr(a, attr::underline));
}

// ── Style tests ─────────────────────────────────────────────────────────

TEST(StyleTest, DefaultIsDefault) {
    style s;
    EXPECT_TRUE(s.is_default());
}

TEST(StyleTest, NonDefaultFg) {
    style s;
    s.fg = colors::red;
    EXPECT_FALSE(s.is_default());
}

TEST(StyleTest, NonDefaultAttrs) {
    style s;
    s.attrs = attr::bold;
    EXPECT_FALSE(s.is_default());
}

TEST(StyleTest, Equality) {
    style a{colors::red, colors::blue, attr::bold};
    style b{colors::red, colors::blue, attr::bold};
    style c{colors::red, colors::blue, attr::italic};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(StyleTest, SizeIs10Bytes) {
    static_assert(sizeof(style) == 10);
}

// ── Hash tests ──────────────────────────────────────────────────────────

TEST(HashTest, ColorHashDeterministic) {
    auto h1 = std::hash<color>{}(colors::red);
    auto h2 = std::hash<color>{}(colors::red);
    EXPECT_EQ(h1, h2);
}

TEST(HashTest, ColorHashDiffers) {
    auto h1 = std::hash<color>{}(colors::red);
    auto h2 = std::hash<color>{}(colors::blue);
    EXPECT_NE(h1, h2);
}

TEST(HashTest, StyleHashDeterministic) {
    style s{colors::red, color::default_color(), attr::bold};
    auto h1 = std::hash<style>{}(s);
    auto h2 = std::hash<style>{}(s);
    EXPECT_EQ(h1, h2);
}

// ── StyleTable tests ────────────────────────────────────────────────────

TEST(StyleTableTest, DefaultStyleAtZero) {
    style_table tbl;
    EXPECT_EQ(tbl.size(), 1u);
    EXPECT_TRUE(tbl.lookup(0).is_default());
}

TEST(StyleTableTest, InternReturnsConsistentId) {
    style_table tbl;
    style s{colors::red, color::default_color(), attr::bold};
    auto id1 = tbl.intern(s);
    auto id2 = tbl.intern(s);
    EXPECT_EQ(id1, id2);
    EXPECT_EQ(tbl.size(), 2u);
}

TEST(StyleTableTest, DifferentStylesDifferentIds) {
    style_table tbl;
    style a{colors::red, color::default_color(), attr::none};
    style b{colors::blue, color::default_color(), attr::none};
    auto id_a = tbl.intern(a);
    auto id_b = tbl.intern(b);
    EXPECT_NE(id_a, id_b);
}

TEST(StyleTableTest, LookupRoundTrip) {
    style_table tbl;
    style s{color::from_rgb(10, 20, 30), colors::white, attr::italic | attr::underline};
    auto id = tbl.intern(s);
    EXPECT_EQ(tbl.lookup(id), s);
}

TEST(StyleTableTest, DefaultStyleInternReturnsZero) {
    style_table tbl;
    auto id = tbl.intern(style{});
    EXPECT_EQ(id, default_style_id);
}

TEST(StyleTableTest, ClearResetsToDefault) {
    style_table tbl;
    (void)tbl.intern(style{colors::red, color::default_color(), attr::bold});
    (void)tbl.intern(style{colors::blue, color::default_color(), attr::italic});
    EXPECT_EQ(tbl.size(), 3u);
    tbl.clear();
    EXPECT_EQ(tbl.size(), 1u);
    EXPECT_TRUE(tbl.lookup(0).is_default());
}

TEST(StyleTableTest, ManyStyles) {
    style_table tbl;
    for (uint16_t i = 0; i < 256; ++i) {
        style s{color::from_index_256(static_cast<uint8_t>(i)), color::default_color(), attr::none};
        auto id = tbl.intern(s);
        EXPECT_EQ(tbl.lookup(id), s);
    }
    EXPECT_EQ(tbl.size(), 257u);
}
