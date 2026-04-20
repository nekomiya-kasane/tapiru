/**
 * @file theme_test.cpp
 * @brief Tests for theme/style sheet system.
 */

#include <gtest/gtest.h>
#include "tapiru/core/theme.h"

using namespace tapiru;

TEST(ThemeTest, DefineAndLookup) {
    theme th;
    th.define("danger", {color::from_rgb(255, 0, 0), color::default_color(), attr::bold});
    auto* s = th.lookup("danger");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->fg.r, 255);
    EXPECT_EQ(s->fg.g, 0);
    EXPECT_EQ(s->attrs, attr::bold);
}

TEST(ThemeTest, LookupMissing) {
    theme th;
    EXPECT_EQ(th.lookup("nonexistent"), nullptr);
}

TEST(ThemeTest, Has) {
    theme th;
    th.define("info", {color::from_rgb(0, 0, 255), color::default_color(), attr::none});
    EXPECT_TRUE(th.has("info"));
    EXPECT_FALSE(th.has("warning"));
}

TEST(ThemeTest, Remove) {
    theme th;
    th.define("x", {});
    EXPECT_TRUE(th.has("x"));
    th.remove("x");
    EXPECT_FALSE(th.has("x"));
}

TEST(ThemeTest, Clear) {
    theme th;
    th.define("a", {});
    th.define("b", {});
    EXPECT_EQ(th.size(), 2u);
    th.clear();
    EXPECT_EQ(th.size(), 0u);
}

TEST(ThemeTest, OverwriteExisting) {
    theme th;
    th.define("x", {color::from_rgb(1, 2, 3), color::default_color(), attr::none});
    th.define("x", {color::from_rgb(4, 5, 6), color::default_color(), attr::bold});
    auto* s = th.lookup("x");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->fg.r, 4);
    EXPECT_EQ(s->attrs, attr::bold);
}

// ── Preset themes ──────────────────────────────────────────────────────

TEST(ThemeTest, DarkPreset) {
    auto th = theme::dark();
    EXPECT_TRUE(th.has("danger"));
    EXPECT_TRUE(th.has("error"));
    EXPECT_TRUE(th.has("warning"));
    EXPECT_TRUE(th.has("success"));
    EXPECT_TRUE(th.has("info"));
    EXPECT_TRUE(th.has("muted"));
    EXPECT_TRUE(th.has("accent"));
    EXPECT_TRUE(th.has("highlight"));
    EXPECT_TRUE(th.has("title"));
    EXPECT_TRUE(th.has("link"));
    EXPECT_TRUE(th.has("text"));
    EXPECT_GE(th.size(), 10u);
}

TEST(ThemeTest, LightPreset) {
    auto th = theme::light();
    EXPECT_TRUE(th.has("danger"));
    EXPECT_TRUE(th.has("link"));
    auto* link = th.lookup("link");
    ASSERT_NE(link, nullptr);
    EXPECT_EQ(link->attrs, attr::underline);
}

TEST(ThemeTest, MonokaiPreset) {
    auto th = theme::monokai();
    EXPECT_TRUE(th.has("danger"));
    auto* danger = th.lookup("danger");
    ASSERT_NE(danger, nullptr);
    EXPECT_EQ(danger->attrs, attr::bold);
}

TEST(ThemeTest, DarkDangerIsRed) {
    auto th = theme::dark();
    auto* s = th.lookup("danger");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->fg.r, 255);
    EXPECT_EQ(s->fg.g, 85);
    EXPECT_EQ(s->fg.b, 85);
}
