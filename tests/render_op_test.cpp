#include "tapiru/core/console.h"
#include "tapiru/text/render_op.h"

#include <gtest/gtest.h>

using namespace tapiru;

// Macro helper: compile_markup is consteval, so the string literal must appear
// directly at the call site. This macro creates a local plan + context and
// executes it, storing the result in `varname`.
#define RENDER_NO_COLOR(varname, literal, w)                                                                           \
    std::string varname;                                                                                               \
    do {                                                                                                               \
        constexpr auto _plan_ = compile_markup(literal);                                                               \
        markup_render_context _ctx_;                                                                                   \
        _ctx_.src = literal;                                                                                           \
        _ctx_.width = (w);                                                                                             \
        _ctx_.color_on = false;                                                                                        \
        _ctx_.emitter = nullptr;                                                                                       \
        _ctx_.out = &varname;                                                                                          \
        execute_rich(_plan_, _ctx_);                                                                                   \
    } while (0)

// ── Pure compile-time ops ───────────────────────────────────────────────

TEST(RenderOpTest, LineBreak) {
    RENDER_NO_COLOR(out, "[br]", 40);
    EXPECT_EQ(out, "\n");
}

TEST(RenderOpTest, Spacing) {
    RENDER_NO_COLOR(out, "[sp=5]", 40);
    EXPECT_EQ(out, "     ");
}

TEST(RenderOpTest, InlineText) {
    RENDER_NO_COLOR(out, "[bold]Hello[/]", 40);
    EXPECT_EQ(out, "Hello");
}

TEST(RenderOpTest, HeadingText) {
    RENDER_NO_COLOR(out, "[h1]Title[/h1]", 40);
    EXPECT_EQ(out, "Title");
}

TEST(RenderOpTest, LinkEmitsOSC8WhenColorOn) {
    constexpr auto plan = compile_markup("[link=https://x.com]click[/link]");
    std::string out;
    ansi_emitter em;
    markup_render_context ctx;
    ctx.src = "[link=https://x.com]click[/link]";
    ctx.width = 80;
    ctx.color_on = true;
    ctx.emitter = &em;
    ctx.out = &out;
    execute_rich(plan, ctx);
    EXPECT_NE(out.find("\033]8;;https://x.com\033\\"), std::string::npos);
    EXPECT_NE(out.find("click"), std::string::npos);
    EXPECT_NE(out.find("\033]8;;\033\\"), std::string::npos);
}

// ── Width-dependent ops ─────────────────────────────────────────────────

TEST(RenderOpTest, RuleNoTitle) {
    RENDER_NO_COLOR(out, "[rule]", 10);
    // Should be 10 horizontal rule chars (─ is 3 bytes in UTF-8)
    EXPECT_EQ(out.size(), 10u * 3u);
}

TEST(RenderOpTest, RuleWithTitle) {
    RENDER_NO_COLOR(out, "[rule=Test]", 20);
    EXPECT_NE(out.find("Test"), std::string::npos);
}

TEST(RenderOpTest, BoxContainsText) {
    RENDER_NO_COLOR(out, "[box]Hello[/box]", 20);
    EXPECT_NE(out.find("Hello"), std::string::npos);
    size_t newlines = 0;
    for (char c : out) {
        if (c == '\n') {
            ++newlines;
        }
    }
    EXPECT_GE(newlines, 2u);
}

TEST(RenderOpTest, IndentAddsSpaces) {
    RENDER_NO_COLOR(out, "[indent=4]text[/indent]", 40);
    EXPECT_EQ(out.substr(0, 4), "    ");
    EXPECT_NE(out.find("text"), std::string::npos);
}

TEST(RenderOpTest, QuoteHasVerticalBar) {
    RENDER_NO_COLOR(out, "[quote]text[/quote]", 40);
    // │ is U+2502 = E2 94 82 in UTF-8
    EXPECT_NE(out.find("\xe2\x94\x82"), std::string::npos);
    EXPECT_NE(out.find("text"), std::string::npos);
}

TEST(RenderOpTest, CodeHasIndent) {
    RENDER_NO_COLOR(out, "[code]fn()[/code]", 40);
    EXPECT_EQ(out.substr(0, 2), "  ");
    EXPECT_NE(out.find("fn()"), std::string::npos);
}

TEST(RenderOpTest, ProgressBar50) {
    RENDER_NO_COLOR(out, "[progress=50]", 20);
    EXPECT_NE(out.find("50%"), std::string::npos);
    EXPECT_NE(out.find("\xe2\x96\x88"), std::string::npos); // █
    EXPECT_NE(out.find("\xe2\x96\x91"), std::string::npos); // ░
}

TEST(RenderOpTest, FractionBar) {
    RENDER_NO_COLOR(out, "[bar=3/5]", 20);
    EXPECT_NE(out.find("3/5"), std::string::npos);
}

TEST(RenderOpTest, EmojiResolvesAtRuntime) {
    RENDER_NO_COLOR(out, "[emoji=fire]", 40);
    // 🔥 = U+1F525 = F0 9F 94 A5 in UTF-8
    EXPECT_EQ(out, "\xf0\x9f\x94\xa5");
}

TEST(RenderOpTest, BrSpCombined) {
    RENDER_NO_COLOR(out, "[br][sp=3]", 40);
    EXPECT_EQ(out, "\n   ");
}

TEST(RenderOpTest, SemanticErrorNoColor) {
    RENDER_NO_COLOR(out, "[error]E[/][success]S[/]", 40);
    EXPECT_EQ(out, "ES");
}

TEST(RenderOpTest, BadgeNoColor) {
    RENDER_NO_COLOR(out, "[badge.red]ERR[/]", 40);
    EXPECT_EQ(out, "ERR");
}

// ── Console print_rich (constexpr path) ─────────────────────────────────

TEST(RenderOpTest, ConsolePrintRich) {
    std::string captured;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { captured += s; };
    cfg.no_color = true;
    console con(cfg);

    TAPIRU_PRINT_RICH(con, "[bold]Hello[/] [rule]");
    EXPECT_NE(captured.find("Hello"), std::string::npos);
}

TEST(RenderOpTest, ConsolePrintRichBox) {
    std::string captured;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { captured += s; };
    cfg.no_color = true;
    console con(cfg);

    TAPIRU_PRINT_RICH(con, "[box]Content[/box]");
    EXPECT_NE(captured.find("Content"), std::string::npos);
}

// ── Round-trip: pure CT ops produce same output regardless of width ──────

TEST(RenderOpTest, PureCTOpsWidthIndependent) {
    RENDER_NO_COLOR(out1, "[br][sp=3]", 40);
    RENDER_NO_COLOR(out2, "[br][sp=3]", 120);
    EXPECT_EQ(out1, out2);
}
