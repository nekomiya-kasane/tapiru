#include "tapiru/core/terminal.h"

#include <gtest/gtest.h>

using namespace tapiru;

TEST(TerminalTest, GetSizeReturnsNonZero) {
    auto sz = terminal::get_size();
    // In CI or non-TTY environments, fallback is 80x24
    EXPECT_GT(sz.width, 0u);
    EXPECT_GT(sz.height, 0u);
}

TEST(TerminalTest, DetectColorDepthReturnsValid) {
    auto depth = terminal::detect_color_depth();
    // Just verify it returns a valid enum value
    EXPECT_GE(static_cast<uint8_t>(depth), 0);
    EXPECT_LE(static_cast<uint8_t>(depth), 3);
}

TEST(TerminalTest, IsTtyStdout) {
    // In test environments this may be true or false, just ensure no crash
    [[maybe_unused]] bool result = terminal::is_tty(stdout);
}

TEST(TerminalTest, EnableVtProcessing) {
    // Should not crash regardless of platform
    [[maybe_unused]] bool result = terminal::enable_vt_processing();
}
