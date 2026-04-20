#include <gtest/gtest.h>

#include "tapiru/core/console.h"

using namespace tapiru;

// ── VirtualTerminal: test mock that captures ANSI output ────────────────

class virtual_terminal {
public:
    /** @brief Create a console that writes to this virtual terminal. */
    [[nodiscard]] console make_console(bool color = true) {
        console_config cfg;
        cfg.sink = [this](std::string_view data) {
            buffer_ += data;
        };
        cfg.depth = color ? color_depth::true_color : color_depth::none;
        cfg.force_color = color;
        cfg.no_color = !color;
        return console(cfg);
    }

    /** @brief Get the raw captured output (including ANSI sequences). */
    [[nodiscard]] const std::string& raw() const noexcept { return buffer_; }

    /** @brief Strip ANSI escape sequences from the captured output. */
    [[nodiscard]] std::string plain() const {
        std::string result;
        result.reserve(buffer_.size());
        size_t i = 0;
        while (i < buffer_.size()) {
            if (buffer_[i] == '\033' && i + 1 < buffer_.size() && buffer_[i + 1] == '[') {
                // Skip until 'm' or end
                i += 2;
                while (i < buffer_.size() && buffer_[i] != 'm') ++i;
                if (i < buffer_.size()) ++i;  // skip 'm'
            } else {
                result += buffer_[i];
                ++i;
            }
        }
        return result;
    }

    /** @brief Check if the raw output contains a specific ANSI sequence. */
    [[nodiscard]] bool contains_ansi(std::string_view seq) const {
        return buffer_.find(seq) != std::string::npos;
    }

    /** @brief Clear the buffer. */
    void clear() { buffer_.clear(); }

private:
    std::string buffer_;
};

// ── Console: plain text ─────────────────────────────────────────────────

TEST(ConsoleTest, PlainTextPrint) {
    virtual_terminal vt;
    auto con = vt.make_console();
    con.print("Hello World");
    EXPECT_EQ(vt.plain(), "Hello World\n");
}

TEST(ConsoleTest, PlainTextNoNewline) {
    virtual_terminal vt;
    auto con = vt.make_console();
    con.print_inline("Hello");
    EXPECT_EQ(vt.plain(), "Hello");
}

TEST(ConsoleTest, WriteBypassesMarkup) {
    virtual_terminal vt;
    auto con = vt.make_console();
    con.write("[bold]not parsed");
    EXPECT_EQ(vt.raw(), "[bold]not parsed");
}

TEST(ConsoleTest, Newline) {
    virtual_terminal vt;
    auto con = vt.make_console();
    con.newline();
    EXPECT_EQ(vt.raw(), "\n");
}

// ── Console: styled output ──────────────────────────────────────────────

TEST(ConsoleTest, BoldEmitsAnsi) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("[bold]Hello[/]");

    // Should contain bold SGR (code 1)
    EXPECT_TRUE(vt.contains_ansi("\033[1m"));
    // Should contain reset
    EXPECT_TRUE(vt.contains_ansi("\033[0m"));
    // Plain text should be just "Hello\n"
    EXPECT_EQ(vt.plain(), "Hello\n");
}

TEST(ConsoleTest, RedFgEmitsAnsi) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("[red]Error[/]");

    EXPECT_TRUE(vt.contains_ansi("\033[31m"));
    EXPECT_EQ(vt.plain(), "Error\n");
}

TEST(ConsoleTest, CompoundStyleEmitsAnsi) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("[bold red on_blue]Alert[/]");

    EXPECT_TRUE(vt.contains_ansi("1"));   // bold
    EXPECT_TRUE(vt.contains_ansi("31"));  // red fg
    EXPECT_TRUE(vt.contains_ansi("44"));  // blue bg
    EXPECT_EQ(vt.plain(), "Alert\n");
}

TEST(ConsoleTest, HexColorEmitsAnsi) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("[#FF8000]Orange[/]");

    EXPECT_TRUE(vt.contains_ansi("38;2;255;128;0"));
    EXPECT_EQ(vt.plain(), "Orange\n");
}

// ── Console: no-color mode ──────────────────────────────────────────────

TEST(ConsoleTest, NoColorMode) {
    virtual_terminal vt;
    auto con = vt.make_console(false);
    con.print("[bold red]Hello[/]");

    // Should NOT contain any ANSI sequences
    EXPECT_EQ(vt.raw(), "Hello\n");
}

// ── Console: mixed content ──────────────────────────────────────────────

TEST(ConsoleTest, MixedContent) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("Hello [bold]World[/bold]!");

    EXPECT_EQ(vt.plain(), "Hello World!\n");
    EXPECT_TRUE(vt.contains_ansi("\033[1m"));
}

// ── Console: multiple prints ────────────────────────────────────────────

TEST(ConsoleTest, MultiplePrints) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("[red]Line 1[/]");
    con.print("[blue]Line 2[/]");

    auto plain = vt.plain();
    EXPECT_TRUE(plain.find("Line 1") != std::string::npos);
    EXPECT_TRUE(plain.find("Line 2") != std::string::npos);
}

// ── Console: escaped brackets ───────────────────────────────────────────

TEST(ConsoleTest, EscapedBrackets) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("Use [[bold] for bold");

    EXPECT_EQ(vt.plain(), "Use [bold] for bold\n");
}

// ── VirtualTerminal: ANSI stripping ─────────────────────────────────────

TEST(VirtualTerminalTest, StripAnsi) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("[bold italic red on_blue]Complex[/]");

    auto plain = vt.plain();
    EXPECT_EQ(plain, "Complex\n");
    // Raw should be longer due to ANSI sequences
    EXPECT_GT(vt.raw().size(), plain.size());
}

TEST(VirtualTerminalTest, Clear) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    con.print("Hello");
    EXPECT_FALSE(vt.raw().empty());
    vt.clear();
    EXPECT_TRUE(vt.raw().empty());
}

// ── Console: color_enabled ──────────────────────────────────────────────

TEST(ConsoleTest, ColorEnabledTrue) {
    virtual_terminal vt;
    auto con = vt.make_console(true);
    EXPECT_TRUE(con.color_enabled());
}

TEST(ConsoleTest, ColorEnabledFalse) {
    virtual_terminal vt;
    auto con = vt.make_console(false);
    EXPECT_FALSE(con.color_enabled());
}
