#include "tapiru/core/console.h"
#include "tapiru/text/constexpr_markup.h"
#include "tapiru/text/emoji.h"
#include "tapiru/text/markdown.h"
#include "tapiru/text/markup.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/spinner.h"

#include <chrono>
#include <cstdio>
#include <gtest/gtest.h>
#include <mutex>
#include <string>

using namespace tapiru;
using clk = std::chrono::high_resolution_clock;

// ── Helpers ─────────────────────────────────────────────────────────────

class null_sink {
  public:
    void operator()(std::string_view) {}
};

static console make_null_console() {
    console_config cfg;
    cfg.sink = [](std::string_view) {};
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    return console(cfg);
}

template <typename Fn> double measure_us(Fn &&fn, int iterations = 1000) {
    auto start = clk::now();
    for (int i = 0; i < iterations; ++i) {
        fn();
    }
    auto end = clk::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return static_cast<double>(ns) / (iterations * 1000.0); // microseconds per call
}

// ── Markup parsing benchmarks ───────────────────────────────────────────

TEST(BenchmarkTest, MarkupParseSimple) {
    double us = measure_us([] {
        auto frags = parse_markup("[bold red]Hello World[/]");
        (void)frags;
    });
    std::printf("  markup parse (simple):    %.2f us/call\n", us);
    EXPECT_LT(us, 500.0); // sanity: under 500us
}

TEST(BenchmarkTest, MarkupParseComplex) {
    double us = measure_us([] {
        auto frags = parse_markup("[bold]Title[/bold]: [italic cyan]description[/italic cyan] "
                                  "[dim]([/dim][#FF8000]v1.2.3[/#FF8000][dim])[/dim]");
        (void)frags;
    });
    std::printf("  markup parse (complex):   %.2f us/call\n", us);
    EXPECT_LT(us, 1000.0);
}

TEST(BenchmarkTest, StripMarkup) {
    double us = measure_us([] {
        auto s = strip_markup("[bold red]Hello[/] [italic]World[/]");
        (void)s;
    });
    std::printf("  strip markup:             %.2f us/call\n", us);
    EXPECT_LT(us, 500.0);
}

// ── Emoji benchmarks ────────────────────────────────────────────────────

TEST(BenchmarkTest, EmojiLookup) {
    double us = measure_us(
        [] {
            auto cp = emoji_lookup(":fire:");
            (void)cp;
        },
        10000);
    std::printf("  emoji lookup:             %.2f us/call\n", us);
    EXPECT_LT(us, 50.0);
}

TEST(BenchmarkTest, EmojiReplace) {
    double us = measure_us([] {
        auto s = replace_emoji(":fire: Hot :rocket: Launch :star: Rating");
        (void)s;
    });
    std::printf("  emoji replace (3 codes):  %.2f us/call\n", us);
    EXPECT_LT(us, 500.0);
}

// ── Markdown benchmarks ─────────────────────────────────────────────────

TEST(BenchmarkTest, MarkdownParse) {
    double us = measure_us([] {
        auto blocks = parse_markdown("# Title\n\nSome **bold** and *italic* text.\n\n"
                                     "- Item one\n- Item two\n- Item three\n\n"
                                     "---\n\n> A blockquote\n\nFinal paragraph.");
        (void)blocks;
    });
    std::printf("  markdown parse:           %.2f us/call\n", us);
    EXPECT_LT(us, 1000.0);
}

TEST(BenchmarkTest, MarkdownInlineConvert) {
    double us = measure_us([] {
        auto s = md_inline_to_markup("Hello **bold** and *italic* and `code` world");
        (void)s;
    });
    std::printf("  md inline to markup:      %.2f us/call\n", us);
    EXPECT_LT(us, 500.0);
}

// ── Widget rendering benchmarks ─────────────────────────────────────────

TEST(BenchmarkTest, TextWidgetRender) {
    auto con = make_null_console();
    double us = measure_us([&] { con.print_widget(text_builder("[bold red]Hello World[/]"), 80); });
    std::printf("  text widget render:       %.2f us/call\n", us);
    EXPECT_LT(us, 5000.0);
}

TEST(BenchmarkTest, PanelWidgetRender) {
    auto con = make_null_console();
    double us = measure_us([&] {
        panel_builder pb(text_builder("Panel content here"));
        pb.border(border_style::rounded);
        pb.title("Title");
        con.print_widget(std::move(pb), 60);
    });
    std::printf("  panel widget render:      %.2f us/call\n", us);
    EXPECT_LT(us, 10000.0);
}

TEST(BenchmarkTest, ProgressWidgetRender) {
    auto con = make_null_console();
    auto task = std::make_shared<progress_task>("Loading", 100);
    task->advance(50);
    double us = measure_us([&] {
        con.print_widget(progress_builder().add_task(task).bar_width(20).complete_char('#').remaining_char('.'), 80);
    });
    std::printf("  progress widget render:   %.2f us/call\n", us);
    EXPECT_LT(us, 5000.0);
}

TEST(BenchmarkTest, MarkdownWidgetRender) {
    auto con = make_null_console();
    double us =
        measure_us([&] { con.print_widget(markdown_builder("# Title\n\nSome **bold** text\n\n- Item\n\n---"), 80); });
    std::printf("  markdown widget render:   %.2f us/call\n", us);
    EXPECT_LT(us, 10000.0);
}
