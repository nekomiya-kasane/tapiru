#include "tapiru/core/unicode_width.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── UTF-8 decode ────────────────────────────────────────────────────────

TEST(Utf8DecodeTest, Ascii) {
    char32_t cp;
    EXPECT_EQ(utf8_decode("A", 1, cp), 1);
    EXPECT_EQ(cp, U'A');
}

TEST(Utf8DecodeTest, TwoByte) {
    // é = U+00E9 = 0xC3 0xA9
    const char data[] = "\xC3\xA9";
    char32_t cp;
    EXPECT_EQ(utf8_decode(data, 2, cp), 2);
    EXPECT_EQ(cp, U'\x00E9');
}

TEST(Utf8DecodeTest, ThreeByte) {
    // 中 = U+4E2D = 0xE4 0xB8 0xAD
    const char data[] = "\xE4\xB8\xAD";
    char32_t cp;
    EXPECT_EQ(utf8_decode(data, 3, cp), 3);
    EXPECT_EQ(cp, U'\x4E2D');
}

TEST(Utf8DecodeTest, FourByte) {
    // 😀 = U+1F600 = 0xF0 0x9F 0x98 0x80
    const char data[] = "\xF0\x9F\x98\x80";
    char32_t cp;
    EXPECT_EQ(utf8_decode(data, 4, cp), 4);
    EXPECT_EQ(cp, U'\x1F600');
}

TEST(Utf8DecodeTest, InvalidByte) {
    const char data[] = "\xFF";
    char32_t cp;
    int n = utf8_decode(data, 1, cp);
    EXPECT_EQ(n, 1);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8DecodeTest, TruncatedSequence) {
    // Start of 3-byte but only 1 byte available
    const char data[] = "\xE4";
    char32_t cp;
    int n = utf8_decode(data, 1, cp);
    EXPECT_EQ(n, 1);
    EXPECT_EQ(cp, U'\xFFFD');
}

// ── char_width ──────────────────────────────────────────────────────────

TEST(CharWidthTest, AsciiPrintable) {
    EXPECT_EQ(char_width(U'A'), 1);
    EXPECT_EQ(char_width(U' '), 1);
    EXPECT_EQ(char_width(U'~'), 1);
}

TEST(CharWidthTest, ControlChars) {
    EXPECT_EQ(char_width(U'\x00'), -1); // NUL
    EXPECT_EQ(char_width(U'\x01'), -1); // SOH
    EXPECT_EQ(char_width(U'\x7F'), -1); // DEL
    EXPECT_EQ(char_width(U'\x80'), -1); // C1
    EXPECT_EQ(char_width(U'\x9F'), -1); // C1 end
}

TEST(CharWidthTest, CjkIdeograph) {
    EXPECT_EQ(char_width(U'\x4E2D'), 2); // 中
    EXPECT_EQ(char_width(U'\x4E00'), 2); // 一
    EXPECT_EQ(char_width(U'\x9FFF'), 2); // CJK range
}

TEST(CharWidthTest, Hiragana) {
    EXPECT_EQ(char_width(U'\x3042'), 2); // あ
    EXPECT_EQ(char_width(U'\x3044'), 2); // い
}

TEST(CharWidthTest, Katakana) {
    EXPECT_EQ(char_width(U'\x30A2'), 2); // ア
    EXPECT_EQ(char_width(U'\x30AB'), 2); // カ
}

TEST(CharWidthTest, HangulSyllable) {
    EXPECT_EQ(char_width(U'\xAC00'), 2); // 가
    EXPECT_EQ(char_width(U'\xD7A3'), 2); // last Hangul syllable
}

TEST(CharWidthTest, FullwidthForms) {
    EXPECT_EQ(char_width(U'\xFF01'), 2); // ！
    EXPECT_EQ(char_width(U'\xFF10'), 2); // ０
}

TEST(CharWidthTest, CombiningMark) {
    EXPECT_EQ(char_width(U'\x0300'), 0); // Combining Grave Accent
    EXPECT_EQ(char_width(U'\x0301'), 0); // Combining Acute Accent
    EXPECT_EQ(char_width(U'\x036F'), 0); // end of combining diacriticals
}

TEST(CharWidthTest, ZeroWidthChars) {
    EXPECT_EQ(char_width(U'\x200B'), 0); // Zero Width Space
    EXPECT_EQ(char_width(U'\x200D'), 0); // Zero Width Joiner
    EXPECT_EQ(char_width(U'\xFEFF'), 0); // BOM
    EXPECT_EQ(char_width(U'\xFE0F'), 0); // Variation Selector-16
}

TEST(CharWidthTest, Emoji) {
    EXPECT_EQ(char_width(U'\x1F600'), 2); // 😀
    EXPECT_EQ(char_width(U'\x1F4A9'), 2); // 💩
    EXPECT_EQ(char_width(U'\x2764'), 2);  // ❤
}

TEST(CharWidthTest, HangulJamo) {
    EXPECT_EQ(char_width(U'\x1100'), 2); // ᄀ (Hangul Choseong)
    EXPECT_EQ(char_width(U'\x115F'), 2); // last Choseong
}

TEST(CharWidthTest, HangulJungseongZeroWidth) {
    EXPECT_EQ(char_width(U'\x1160'), 0); // Hangul Jungseong Filler
    EXPECT_EQ(char_width(U'\x11FF'), 0); // last Jongseong
}

// ── string_width ────────────────────────────────────────────────────────

TEST(StringWidthTest, AsciiString) {
    EXPECT_EQ(string_width("Hello"), 5);
}

TEST(StringWidthTest, EmptyString) {
    EXPECT_EQ(string_width(""), 0);
}

TEST(StringWidthTest, CjkString) {
    // "中文" = 2 + 2 = 4
    EXPECT_EQ(string_width("\xE4\xB8\xAD\xE6\x96\x87"), 4);
}

TEST(StringWidthTest, MixedAsciiCjk) {
    // "Hi中文" = 1 + 1 + 2 + 2 = 6
    EXPECT_EQ(string_width("Hi\xE4\xB8\xAD\xE6\x96\x87"), 6);
}

TEST(StringWidthTest, EmojiString) {
    // "😀" = 2
    EXPECT_EQ(string_width("\xF0\x9F\x98\x80"), 2);
}

TEST(StringWidthTest, CombiningCharacters) {
    // "é" as e + combining acute = 1 + 0 = 1
    EXPECT_EQ(string_width("e\xCC\x81"), 1);
}

TEST(StringWidthTest, FullwidthPunctuation) {
    // "！" = 2
    EXPECT_EQ(string_width("\xEF\xBC\x81"), 2);
}

TEST(StringWidthTest, JapaneseHiragana) {
    // "あいう" = 2 + 2 + 2 = 6
    EXPECT_EQ(string_width("\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86"), 6);
}
