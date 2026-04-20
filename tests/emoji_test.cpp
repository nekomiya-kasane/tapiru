#include <gtest/gtest.h>

#include "tapiru/text/emoji.h"

using namespace tapiru;

// ── emoji_lookup ────────────────────────────────────────────────────────

TEST(EmojiTest, LookupWithColons) {
    EXPECT_EQ(emoji_lookup(":fire:"), U'\U0001F525');
    EXPECT_EQ(emoji_lookup(":heart:"), U'\x2764');
    EXPECT_EQ(emoji_lookup(":rocket:"), U'\U0001F680');
    EXPECT_EQ(emoji_lookup(":star:"), U'\x2B50');
    EXPECT_EQ(emoji_lookup(":wave:"), U'\U0001F44B');
}

TEST(EmojiTest, LookupWithoutColons) {
    EXPECT_EQ(emoji_lookup("fire"), U'\U0001F525');
    EXPECT_EQ(emoji_lookup("smile"), U'\U0001F604');
}

TEST(EmojiTest, LookupUnknown) {
    EXPECT_EQ(emoji_lookup(":nonexistent:"), 0u);
    EXPECT_EQ(emoji_lookup(""), 0u);
    EXPECT_EQ(emoji_lookup("::"), 0u);
}

TEST(EmojiTest, LookupAllCategories) {
    // Faces
    EXPECT_NE(emoji_lookup(":smile:"), 0u);
    EXPECT_NE(emoji_lookup(":grin:"), 0u);
    EXPECT_NE(emoji_lookup(":cry:"), 0u);
    EXPECT_NE(emoji_lookup(":thinking:"), 0u);
    // Objects
    EXPECT_NE(emoji_lookup(":bomb:"), 0u);
    EXPECT_NE(emoji_lookup(":key:"), 0u);
    EXPECT_NE(emoji_lookup(":lock:"), 0u);
    // Nature
    EXPECT_NE(emoji_lookup(":sun:"), 0u);
    EXPECT_NE(emoji_lookup(":moon:"), 0u);
    EXPECT_NE(emoji_lookup(":rainbow:"), 0u);
}

// ── replace_emoji ───────────────────────────────────────────────────────

TEST(EmojiTest, ReplaceSimple) {
    auto result = replace_emoji(":fire: Hot!");
    // Should start with the fire emoji UTF-8 bytes, not ":fire:"
    EXPECT_TRUE(result.find(":fire:") == std::string::npos);
    EXPECT_TRUE(result.find("Hot!") != std::string::npos);
}

TEST(EmojiTest, ReplaceMultiple) {
    auto result = replace_emoji(":star: :heart: :rocket:");
    EXPECT_TRUE(result.find(":star:") == std::string::npos);
    EXPECT_TRUE(result.find(":heart:") == std::string::npos);
    EXPECT_TRUE(result.find(":rocket:") == std::string::npos);
}

TEST(EmojiTest, ReplaceUnknownPassthrough) {
    auto result = replace_emoji(":unknown_code: stays");
    EXPECT_TRUE(result.find(":unknown_code:") != std::string::npos);
    EXPECT_TRUE(result.find("stays") != std::string::npos);
}

TEST(EmojiTest, ReplaceNoShortcodes) {
    auto result = replace_emoji("plain text");
    EXPECT_EQ(result, "plain text");
}

TEST(EmojiTest, ReplaceMixed) {
    auto result = replace_emoji("Hello :wave: World :fire:!");
    EXPECT_TRUE(result.find(":wave:") == std::string::npos);
    EXPECT_TRUE(result.find(":fire:") == std::string::npos);
    EXPECT_TRUE(result.find("Hello") != std::string::npos);
    EXPECT_TRUE(result.find("World") != std::string::npos);
}

TEST(EmojiTest, ReplaceLoneColon) {
    auto result = replace_emoji("time: 3:00");
    // Should not crash or mangle — lone colons pass through
    EXPECT_TRUE(result.find("time") != std::string::npos);
}
