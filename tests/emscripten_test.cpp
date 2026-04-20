/**
 * @file emscripten_test.cpp
 * @brief Tests for Emscripten backend stubs.
 *
 * These tests verify that the Emscripten backend compiles and provides
 * sensible defaults. On non-Emscripten platforms, they test the stub
 * behavior via a mock layer that simulates the Emscripten API surface.
 *
 * Tagged EMSCRIPTEN_ONLY — on native platforms these test the fallback
 * behavior of the same API surface.
 */

#include <gtest/gtest.h>

#include "tapiru/core/terminal.h"
#include "tapiru/core/raw_input.h"

using namespace tapiru;

// ── EmscriptenTerminalSize ──────────────────────────────────────────────

TEST(EmscriptenTest, TerminalSizeReturnsReasonable) {
    auto sz = terminal::get_size();
    // Must return positive dimensions (at least the fallback 80x24)
    EXPECT_GE(sz.width, 1u);
    EXPECT_GE(sz.height, 1u);
    EXPECT_LE(sz.width, 1000u);   // sanity upper bound
    EXPECT_LE(sz.height, 500u);
}

// ── EmscriptenRawModeNoop ───────────────────────────────────────────────

TEST(EmscriptenTest, RawModeDoesNotCrash) {
    // On Emscripten, raw_mode is a no-op.
    // On native, this tests the normal raw_mode RAII.
    // Either way, constructing and destroying must not crash.
    // We skip actual construction on native CI where there may be no TTY.
    if (!terminal::is_tty()) {
        GTEST_SKIP() << "No TTY available, skipping raw_mode test";
    }
    {
        raw_mode rm;
        // Just verify it doesn't throw or crash
        (void)rm;
    }
    // After destruction, terminal should be restored
}

// ── EmscriptenOutput ────────────────────────────────────────────────────

TEST(EmscriptenTest, OutputFunctionsAvailable) {
    // Verify terminal utility functions are callable and don't crash
    auto depth = terminal::detect_color_depth();
    EXPECT_GE(static_cast<int>(depth), 0);
    EXPECT_LE(static_cast<int>(depth), 3);

    bool vt = terminal::enable_vt_processing();
    (void)vt;  // may be true or false depending on platform

    terminal::enable_utf8();  // must not crash
}
