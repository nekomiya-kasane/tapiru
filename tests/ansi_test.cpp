#include "tapiru/core/ansi.h"
#include "tapiru/core/terminal.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── Low-level builders ──────────────────────────────────────────────────

TEST(AnsiTest, CursorTo) {
    EXPECT_EQ(ansi::cursor_to(1, 1), "\033[1;1H");
    EXPECT_EQ(ansi::cursor_to(10, 20), "\033[10;20H");
}

TEST(AnsiTest, CursorMovement) {
    EXPECT_EQ(ansi::cursor_up(3), "\033[3A");
    EXPECT_EQ(ansi::cursor_down(1), "\033[1B");
    EXPECT_EQ(ansi::cursor_right(5), "\033[5C");
    EXPECT_EQ(ansi::cursor_left(2), "\033[2D");
    EXPECT_EQ(ansi::cursor_up(0), "");
}

// ── SGR from default ────────────────────────────────────────────────────

TEST(AnsiTest, SgrDefaultIsEmpty) {
    EXPECT_EQ(ansi::sgr(style{}), "");
}

TEST(AnsiTest, SgrBoldOnly) {
    style s;
    s.attrs = attr::bold;
    EXPECT_EQ(ansi::sgr(s), "\033[1m");
}

TEST(AnsiTest, SgrFgOnly16) {
    style s;
    s.fg = colors::red; // index 1 → SGR 31
    EXPECT_EQ(ansi::sgr(s), "\033[31m");
}

TEST(AnsiTest, SgrFgBright16) {
    style s;
    s.fg = colors::bright_red; // index 9 → SGR 91
    EXPECT_EQ(ansi::sgr(s), "\033[91m");
}

TEST(AnsiTest, SgrBg16) {
    style s;
    s.bg = colors::blue; // index 4 → SGR 44
    EXPECT_EQ(ansi::sgr(s), "\033[44m");
}

TEST(AnsiTest, SgrBgBright16) {
    style s;
    s.bg = colors::bright_green; // index 10 → SGR 102
    EXPECT_EQ(ansi::sgr(s), "\033[102m");
}

TEST(AnsiTest, SgrFg256) {
    style s;
    s.fg = color::from_index_256(200);
    EXPECT_EQ(ansi::sgr(s), "\033[38;5;200m");
}

TEST(AnsiTest, SgrBg256) {
    style s;
    s.bg = color::from_index_256(42);
    EXPECT_EQ(ansi::sgr(s), "\033[48;5;42m");
}

TEST(AnsiTest, SgrFgRgb) {
    style s;
    s.fg = color::from_rgb(10, 20, 30);
    EXPECT_EQ(ansi::sgr(s), "\033[38;2;10;20;30m");
}

TEST(AnsiTest, SgrBgRgb) {
    style s;
    s.bg = color::from_rgb(255, 128, 0);
    EXPECT_EQ(ansi::sgr(s), "\033[48;2;255;128;0m");
}

TEST(AnsiTest, SgrCombined) {
    style s{colors::red, colors::blue, attr::bold | attr::italic};
    // attrs first, then fg, then bg
    EXPECT_EQ(ansi::sgr(s), "\033[1;3;31;44m");
}

TEST(AnsiTest, SgrDefaultFgParam) {
    // Explicitly setting fg to default should produce "39"
    std::string out;
    ansi::append_fg_params(out, color::default_color());
    EXPECT_EQ(out, "39");
}

TEST(AnsiTest, SgrDefaultBgParam) {
    std::string out;
    ansi::append_bg_params(out, color::default_color());
    EXPECT_EQ(out, "49");
}

// ── Emitter: no-op transition ───────────────────────────────────────────

TEST(EmitterTest, NoOpTransition) {
    ansi_emitter em;
    std::string out;
    em.transition(style{}, out);
    EXPECT_EQ(out, "");
}

// ── Emitter: from default to styled ─────────────────────────────────────

TEST(EmitterTest, DefaultToBold) {
    ansi_emitter em;
    std::string out;
    style target;
    target.attrs = attr::bold;
    em.transition(target, out);
    // Both delta and reset paths produce the same for default→bold
    EXPECT_TRUE(out.find("1") != std::string::npos);
    EXPECT_TRUE(out.find("\033[") != std::string::npos);
}

TEST(EmitterTest, DefaultToRedFg) {
    ansi_emitter em;
    std::string out;
    style target;
    target.fg = colors::red;
    em.transition(target, out);
    EXPECT_TRUE(out.find("31") != std::string::npos);
}

// ── Emitter: delta path (incremental change) ────────────────────────────

TEST(EmitterTest, RedToRedBold) {
    ansi_emitter em;
    std::string out;

    style red;
    red.fg = colors::red;
    em.transition(red, out);
    out.clear();

    // Now add bold — should NOT re-emit fg color
    style red_bold;
    red_bold.fg = colors::red;
    red_bold.attrs = attr::bold;
    em.transition(red_bold, out);

    // Delta should just be the bold attribute
    EXPECT_EQ(out, "\033[1m");
}

TEST(EmitterTest, BoldRedToJustRed) {
    ansi_emitter em;
    std::string out;

    style bold_red{colors::red, color::default_color(), attr::bold};
    em.transition(bold_red, out);
    out.clear();

    // Remove bold — delta should emit "22" (bold off)
    style just_red{colors::red, color::default_color(), attr::none};
    em.transition(just_red, out);

    // Should contain "22" for bold-off, should NOT contain fg params
    EXPECT_TRUE(out.find("22") != std::string::npos);
    EXPECT_EQ(out, "\033[22m");
}

// ── Emitter: reset-vs-delta heuristic ───────────────────────────────────

TEST(EmitterTest, ResetHeuristicPrefersShortPath) {
    ansi_emitter em;
    std::string out;

    // Set up a complex style
    style complex{color::from_rgb(10, 20, 30), color::from_rgb(40, 50, 60),
                  attr::bold | attr::italic | attr::underline};
    em.transition(complex, out);
    out.clear();

    // Transition to default — reset path (\033[0m) is much shorter than delta
    em.transition(style{}, out);
    EXPECT_EQ(out, "\033[0m");
}

// ── Emitter: color change only ──────────────────────────────────────────

TEST(EmitterTest, FgColorChange) {
    ansi_emitter em;
    std::string out;

    style red;
    red.fg = colors::red;
    em.transition(red, out);
    out.clear();

    style blue;
    blue.fg = colors::blue;
    em.transition(blue, out);

    // Delta: only fg changes
    EXPECT_EQ(out, "\033[34m");
}

TEST(EmitterTest, BgColorChange) {
    ansi_emitter em;
    std::string out;

    style bg_red;
    bg_red.bg = colors::red;
    em.transition(bg_red, out);
    out.clear();

    style bg_blue;
    bg_blue.bg = colors::blue;
    em.transition(bg_blue, out);

    EXPECT_EQ(out, "\033[44m");
}

// ── Emitter: reset() method ─────────────────────────────────────────────

TEST(EmitterTest, ResetEmitsWhenStyled) {
    ansi_emitter em;
    std::string out;

    style s;
    s.fg = colors::red;
    em.transition(s, out);
    out.clear();

    em.reset(out);
    EXPECT_EQ(out, "\033[0m");
    EXPECT_TRUE(em.hw_state().is_default());
}

TEST(EmitterTest, ResetNoOpWhenDefault) {
    ansi_emitter em;
    std::string out;
    em.reset(out);
    EXPECT_EQ(out, "");
}

// ── Emitter: invalidate ─────────────────────────────────────────────────

TEST(EmitterTest, InvalidateResetsStateWithoutOutput) {
    ansi_emitter em;
    std::string out;

    style s;
    s.fg = colors::red;
    em.transition(s, out);

    em.invalidate();
    EXPECT_TRUE(em.hw_state().is_default());
}

// ── Emitter: multiple transitions ───────────────────────────────────────

TEST(EmitterTest, MultipleTransitions) {
    ansi_emitter em;
    std::string out;

    style s1{colors::red, color::default_color(), attr::bold};
    em.transition(s1, out);

    style s2{colors::green, color::default_color(), attr::bold};
    em.transition(s2, out);

    style s3{colors::green, color::default_color(), attr::bold | attr::italic};
    em.transition(s3, out);

    em.reset(out);

    // Just verify it doesn't crash and produces non-empty output
    EXPECT_FALSE(out.empty());
    // Final state should be default
    EXPECT_TRUE(em.hw_state().is_default());
}

// ═══════════════════════════════════════════════════════════════════════
// OSC sequence builder tests
// ═══════════════════════════════════════════════════════════════════════

// ── OSC 0/1/2: Window title ─────────────────────────────────────────────

TEST(OscTest, SetTitle) {
    auto s = ansi::set_title("My App");
    EXPECT_EQ(s, "\033]0;My App\033\\");
}

TEST(OscTest, SetTitleEmpty) {
    auto s = ansi::set_title("");
    EXPECT_EQ(s, "\033]0;\033\\");
}

TEST(OscTest, SetIconName) {
    auto s = ansi::set_icon_name("icon");
    EXPECT_EQ(s, "\033]1;icon\033\\");
}

TEST(OscTest, SetWindowTitle) {
    auto s = ansi::set_window_title("Window");
    EXPECT_EQ(s, "\033]2;Window\033\\");
}

// ── OSC 4/104: Palette color ────────────────────────────────────────────

TEST(OscTest, SetPaletteColor) {
    auto s = ansi::set_palette_color(1, 0xff, 0x00, 0x80);
    EXPECT_EQ(s, "\033]4;1;rgb:ff/00/80\033\\");
}

TEST(OscTest, SetPaletteColorZero) {
    auto s = ansi::set_palette_color(0, 0, 0, 0);
    EXPECT_EQ(s, "\033]4;0;rgb:00/00/00\033\\");
}

TEST(OscTest, QueryPaletteColor) {
    auto s = ansi::query_palette_color(42);
    EXPECT_EQ(s, "\033]4;42;?\033\\");
}

TEST(OscTest, ResetPaletteColorAll) {
    auto s = ansi::reset_palette_color();
    EXPECT_EQ(s, "\033]104\033\\");
}

TEST(OscTest, ResetPaletteColorSingle) {
    auto s = ansi::reset_palette_color(5);
    EXPECT_EQ(s, "\033]104;5\033\\");
}

// ── OSC 7: CWD ──────────────────────────────────────────────────────────

TEST(OscTest, ReportCwd) {
    auto s = ansi::report_cwd("localhost", "/home/user/project");
    EXPECT_EQ(s, "\033]7;file://localhost/home/user/project\033\\");
}

TEST(OscTest, ReportCwdNoHost) {
    auto s = ansi::report_cwd("", "/tmp");
    EXPECT_EQ(s, "\033]7;file:///tmp\033\\");
}

// ── OSC 8: Hyperlinks ───────────────────────────────────────────────────

TEST(OscTest, HyperlinkOpenNoId) {
    auto s = ansi::hyperlink_open("https://example.com");
    EXPECT_EQ(s, "\033]8;;https://example.com\033\\");
}

TEST(OscTest, HyperlinkOpenWithId) {
    auto s = ansi::hyperlink_open("https://example.com", "link1");
    EXPECT_EQ(s, "\033]8;id=link1;https://example.com\033\\");
}

TEST(OscTest, HyperlinkClose) {
    EXPECT_STREQ(ansi::hyperlink_close, "\033]8;;\033\\");
}

// ── OSC 9: Notification ─────────────────────────────────────────────────

TEST(OscTest, Notify) {
    auto s = ansi::notify("Build complete!");
    EXPECT_EQ(s, "\033]9;Build complete!\033\\");
}

// ── OSC 10/11/12: fg/bg/cursor color ────────────────────────────────────

TEST(OscTest, SetForegroundColor) {
    auto s = ansi::set_foreground_color(0xff, 0x80, 0x00);
    EXPECT_EQ(s, "\033]10;rgb:ff/80/00\033\\");
}

TEST(OscTest, QueryForegroundColor) {
    EXPECT_STREQ(ansi::query_foreground_color, "\033]10;?\033\\");
}

TEST(OscTest, SetBackgroundColor) {
    auto s = ansi::set_background_color(0x1e, 0x1e, 0x2e);
    EXPECT_EQ(s, "\033]11;rgb:1e/1e/2e\033\\");
}

TEST(OscTest, QueryBackgroundColor) {
    EXPECT_STREQ(ansi::query_background_color, "\033]11;?\033\\");
}

TEST(OscTest, SetCursorColor) {
    auto s = ansi::set_cursor_color(0x00, 0xff, 0x00);
    EXPECT_EQ(s, "\033]12;rgb:00/ff/00\033\\");
}

TEST(OscTest, QueryCursorColor) {
    EXPECT_STREQ(ansi::query_cursor_color, "\033]12;?\033\\");
}

// ── OSC 110/111/112: Reset colors ───────────────────────────────────────

TEST(OscTest, ResetForegroundColor) {
    EXPECT_STREQ(ansi::reset_foreground_color, "\033]110\033\\");
}

TEST(OscTest, ResetBackgroundColor) {
    EXPECT_STREQ(ansi::reset_background_color, "\033]111\033\\");
}

TEST(OscTest, ResetCursorColor) {
    EXPECT_STREQ(ansi::reset_cursor_color, "\033]112\033\\");
}

// ── OSC 52: Clipboard ───────────────────────────────────────────────────

TEST(OscTest, ClipboardWrite) {
    auto s = ansi::clipboard_write("c", "SGVsbG8=");
    EXPECT_EQ(s, "\033]52;c;SGVsbG8=\033\\");
}

TEST(OscTest, ClipboardQuery) {
    auto s = ansi::clipboard_query("c");
    EXPECT_EQ(s, "\033]52;c;?\033\\");
}

TEST(OscTest, ClipboardClear) {
    auto s = ansi::clipboard_clear("c");
    EXPECT_EQ(s, "\033]52;c;\033\\");
}

TEST(OscTest, ClipboardPrimary) {
    auto s = ansi::clipboard_write("p", "dGVzdA==");
    EXPECT_EQ(s, "\033]52;p;dGVzdA==\033\\");
}

// ── OSC 133: Shell integration ──────────────────────────────────────────

TEST(OscTest, PromptStart) {
    EXPECT_STREQ(ansi::prompt_start, "\033]133;A\033\\");
}

TEST(OscTest, CommandStart) {
    EXPECT_STREQ(ansi::command_start, "\033]133;B\033\\");
}

TEST(OscTest, OutputStart) {
    EXPECT_STREQ(ansi::output_start, "\033]133;C\033\\");
}

TEST(OscTest, CommandFinished) {
    auto s = ansi::command_finished(0);
    EXPECT_EQ(s, "\033]133;D;0\033\\");
}

TEST(OscTest, CommandFinishedNonZero) {
    auto s = ansi::command_finished(127);
    EXPECT_EQ(s, "\033]133;D;127\033\\");
}

// ── OSC support detection ───────────────────────────────────────────────

TEST(OscDetectionTest, ReturnsStruct) {
    // Just verify it doesn't crash and returns a valid struct
    auto caps = terminal::detect_osc_support();
    // In CI/test environment (not a TTY), all should be false
    // But we can't guarantee the test env, so just check it's callable
    (void)caps.title;
    (void)caps.hyperlink;
    (void)caps.clipboard;
    SUCCEED();
}

TEST(OscDetectionTest, StructDefaultsAllFalse) {
    terminal::osc_support caps{};
    EXPECT_FALSE(caps.title);
    EXPECT_FALSE(caps.palette);
    EXPECT_FALSE(caps.cwd);
    EXPECT_FALSE(caps.hyperlink);
    EXPECT_FALSE(caps.notify);
    EXPECT_FALSE(caps.color_query);
    EXPECT_FALSE(caps.clipboard);
    EXPECT_FALSE(caps.shell_integration);
}
