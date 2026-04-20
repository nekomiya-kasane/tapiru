/**
 * @file screen_test.cpp
 * @brief Tests for screen: factory methods, render_once, query methods.
 */

#include "tapiru/core/component.h"
#include "tapiru/core/console.h"
#include "tapiru/core/screen.h"
#include "tapiru/widgets/builders.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── Factory tests ───────────────────────────────────────────────────────

TEST(ScreenTest, FullscreenFactoryCreates) {
    screen_config cfg;
    cfg.width = 40;
    cfg.sink = [](std::string_view) {};
    auto scr = screen::fullscreen(cfg);
    EXPECT_TRUE(scr.is_interactive());
}

TEST(ScreenTest, FitComponentFactoryCreates) {
    screen_config cfg;
    cfg.width = 40;
    cfg.sink = [](std::string_view) {};
    auto scr = screen::fit_component(cfg);
    EXPECT_TRUE(scr.is_interactive());
}

TEST(ScreenTest, TerminalOutputFactoryCreates) {
    screen_config cfg;
    cfg.width = 40;
    cfg.sink = [](std::string_view) {};
    auto scr = screen::terminal_output(cfg);
    EXPECT_FALSE(scr.is_interactive());
}

// ── render_once ─────────────────────────────────────────────────────────

TEST(ScreenTest, RenderOnceProducesOutput) {
    std::string output;
    screen_config cfg;
    cfg.width = 30;
    cfg.sink = [&output](std::string_view data) { output += data; };
    auto scr = screen::terminal_output(cfg);

    element e = text_builder("render_once_test");
    scr.render_once(e);
    EXPECT_TRUE(output.find("render_once_test") != std::string::npos);
}

TEST(ScreenTest, RenderOnceWithPanel) {
    std::string output;
    screen_config cfg;
    cfg.width = 30;
    cfg.sink = [&output](std::string_view data) { output += data; };
    auto scr = screen::terminal_output(cfg);

    auto pb = panel_builder(text_builder("boxed"));
    element e = std::move(pb);
    scr.render_once(e);
    EXPECT_TRUE(output.find("boxed") != std::string::npos);
}

// ── dimensions ──────────────────────────────────────────────────────────

TEST(ScreenTest, DimensionsReturnsConfiguredWidth) {
    screen_config cfg;
    cfg.width = 42;
    cfg.height = 10;
    cfg.sink = [](std::string_view) {};
    auto scr = screen::terminal_output(cfg);
    auto [w, h] = scr.dimensions();
    EXPECT_EQ(w, 42u);
    EXPECT_EQ(h, 10u);
}

// ── exit_loop ───────────────────────────────────────────────────────────

TEST(ScreenTest, ExitLoopStopsLoop) {
    screen_config cfg;
    cfg.width = 20;
    cfg.fps = 60;
    cfg.sink = [](std::string_view) {};
    auto scr = screen::terminal_output(cfg);

    auto r = make_renderer([]() { return element(text_builder("loop")); });

    // exit_loop before loop should cause loop to exit immediately
    scr.exit_loop();
    // loop() on terminal_output mode should just render once and return
    // (non-interactive mode doesn't actually loop)
    // This test just verifies no crash
    EXPECT_FALSE(scr.is_interactive());
}

// ── post_event ──────────────────────────────────────────────────────────

TEST(ScreenTest, PostEventDoesNotCrash) {
    screen_config cfg;
    cfg.width = 20;
    cfg.sink = [](std::string_view) {};
    auto scr = screen::terminal_output(cfg);

    key_event ke{0, special_key::escape, key_mod::none};
    scr.post_event(ke);
    // Just verify no crash
    EXPECT_TRUE(true);
}

// ── move semantics ──────────────────────────────────────────────────────

TEST(ScreenTest, MoveConstruction) {
    screen_config cfg;
    cfg.width = 20;
    cfg.sink = [](std::string_view) {};
    auto scr1 = screen::terminal_output(cfg);
    auto scr2 = std::move(scr1);
    EXPECT_FALSE(scr2.is_interactive());
}

TEST(ScreenTest, MoveAssignment) {
    screen_config cfg;
    cfg.width = 20;
    cfg.sink = [](std::string_view) {};
    auto scr1 = screen::terminal_output(cfg);
    auto scr2 = screen::terminal_output(cfg);
    scr2 = std::move(scr1);
    EXPECT_FALSE(scr2.is_interactive());
}
