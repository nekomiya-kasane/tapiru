/**
 * @file demo.cpp
 * @brief Comprehensive demo showcasing all tapiru features.
 *
 * Build:
 *   cmake --build build --target tapiru_demo --config Debug
 * Run:
 *   .\build\bin\Debug\tapiru_demo.exe
 */

#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>

// ── Core ────────────────────────────────────────────────────────────────
#include "tapiru/core/console.h"
#include "tapiru/core/gradient.h"
#include "tapiru/core/live.h"
#include "tapiru/core/logging.h"
#include "tapiru/core/style.h"
#include "tapiru/core/theme.h"

// ── Text ────────────────────────────────────────────────────────────────
#include "tapiru/text/constexpr_markup.h"
#include "tapiru/text/emoji.h"
#include "tapiru/text/markdown.h"
#include "tapiru/text/markup.h"

// ── Element / Decorator ──────────────────────────────────────────────────
#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/core/terminal.h"

// ── Widgets ─────────────────────────────────────────────────────────────
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/canvas_widget.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/gauge.h"
#include "tapiru/widgets/image.h"
#include "tapiru/widgets/interactive.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/paragraph.h"
#include "tapiru/widgets/popup.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/spinner.h"
#include "tapiru/widgets/status_bar.h"

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
//  Helper: section header
// ═══════════════════════════════════════════════════════════════════════

static void section(console &con, const char *title) {
    con.newline();
    con.print_widget(rule_builder(title).rule_style(style{colors::bright_cyan, {}, attr::bold}), 70);
    con.newline();
}

// ═══════════════════════════════════════════════════════════════════════
//  1. Markup & Styled Text
// ═══════════════════════════════════════════════════════════════════════

static void demo_markup(console &con) {
    section(con, " 1. Markup & Styled Text ");

    con.print("[bold]Bold[/bold], [italic]Italic[/italic], [underline]Underline[/underline], [strike]Strike[/strike]");
    con.print("[red]Red[/], [green]Green[/], [blue]Blue[/], [yellow]Yellow[/], [magenta]Magenta[/], [cyan]Cyan[/]");
    con.print("[bold bright_white on_red] ERROR [/] [bold bright_white on_green] SUCCESS [/] [bold black on_yellow] "
              "WARNING [/]");
    con.print("[#FF8000]Orange (RGB)[/], [#00BFFF]Deep Sky Blue (RGB)[/], [on_#333333] Dark BG [/]");
    con.print("[bold red]Compound[/bold red] tags: [bold italic cyan]bold+italic+cyan[/]");
    con.print("Escaped bracket: [[ looks like a literal [");
}

// ═══════════════════════════════════════════════════════════════════════
//  2. Constexpr Markup (compile-time style resolution)
// ═══════════════════════════════════════════════════════════════════════

static void demo_constexpr_markup(console &con) {
    section(con, " 2. Constexpr Markup ");

    // Styles resolved entirely at compile time — zero runtime parsing
    constexpr auto error_style = markup_style("bold red");
    constexpr auto info_style = markup_style("bold cyan");
    constexpr auto warn_style = markup_style("bold yellow on_#332200");

    // Verify at compile time
    static_assert(error_style.fg == colors::red);
    static_assert(has_attr(info_style.attrs, attr::bold));

    con.print("[dim]Styles resolved at compile time via consteval:[/]");
    con.print("[bold red]  error_style[/] = bold + red (verified by static_assert)");
    con.print("[bold cyan]  info_style[/]  = bold + cyan");
    con.print("[bold yellow]  warn_style[/]  = bold + yellow + on_#332200");

    // Full compile-time markup parsing
    constexpr auto parsed = ct_parse_markup("[bold]Hello[/] [italic cyan]World[/]");
    static_assert(parsed.count == 3); // "Hello", " ", "World"

    char buf[128];
    std::snprintf(buf, sizeof(buf), "[dim]ct_parse_markup produced %zu fragments at compile time[/]", parsed.count);
    con.print(buf);
}

// ═══════════════════════════════════════════════════════════════════════
//  3. Text Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_text_widget(console &con) {
    section(con, " 3. Text Widget ");

    con.print_widget(
        text_builder("[bold green]Hello[/], [italic]tapiru[/]! A [underline]rich text[/] library for C++23."), 70);
}

// ═══════════════════════════════════════════════════════════════════════
//  4. Rule Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_rule(console &con) {
    section(con, " 4. Rule Widget ");

    con.print_widget(rule_builder(), 70);
    con.print_widget(rule_builder("Section Title"), 70);
    con.print_widget(rule_builder("Left Aligned").align(justify::left), 70);
    con.print_widget(rule_builder("Custom Char").character(U'='), 70);
    con.print_widget(rule_builder("Styled").rule_style(style{colors::magenta, {}, attr::bold}), 70);
}

// ═══════════════════════════════════════════════════════════════════════
//  5. Panel Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_panel(console &con) {
    section(con, " 5. Panel Widget ");

    panel_builder pb1(text_builder("[bold]Welcome to tapiru![/]\nA modern C++23 rich terminal library."));
    pb1.title("About");
    pb1.border(border_style::rounded);
    con.print_widget(std::move(pb1), 50);

    con.newline();

    panel_builder pb2(text_builder("[red]Error:[/] Connection refused\n[dim]Retry in 5 seconds...[/]"));
    pb2.title("Alert");
    pb2.border(border_style::heavy);
    pb2.border_style_override(style{colors::red, {}, attr::bold});
    con.print_widget(std::move(pb2), 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  6. Padding Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_padding(console &con) {
    section(con, " 6. Padding Widget ");

    padding_builder pad(text_builder("[cyan]Padded content[/] (2 cells all around)"));
    pad.pad(1, 4);
    con.print_widget(std::move(pad), 60);
}

// ═══════════════════════════════════════════════════════════════════════
//  7. Columns Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_columns(console &con) {
    section(con, " 7. Columns Layout ");

    columns_builder cols;
    cols.add(text_builder("[bold red]Column A[/]"), 1);
    cols.add(text_builder("[bold green]Column B[/]"), 1);
    cols.add(text_builder("[bold blue]Column C[/]"), 1);
    cols.gap(2);
    con.print_widget(std::move(cols), 60);
}

// ═══════════════════════════════════════════════════════════════════════
//  8. Table Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_table(console &con) {
    section(con, " 8. Table Widget ");

    table_builder tb;
    tb.add_column("Name", {justify::left, 12, 20});
    tb.add_column("Language", {justify::center, 10, 15});
    tb.add_column("Stars", {justify::right, 6, 10});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});

    tb.add_row({"tapiru", "C++23", "1000"});
    tb.add_row({"rich", "Python", "48000"});
    tb.add_row({"chalk", "JS", "21000"});
    tb.add_row({"termcolor", "Rust", "2000"});

    con.print_widget(tb, 60);
}

// ═══════════════════════════════════════════════════════════════════════
//  9. Emoji Shortcodes
// ═══════════════════════════════════════════════════════════════════════

static void demo_emoji(console &con) {
    section(con, " 9. Emoji Shortcodes ");

    con.print(
        replace_emoji(":rocket: [bold]Launch[/] — :fire: [red]Hot[/] — :star: [yellow]Star[/] — :heart: [red]Love[/]"));
    con.print(replace_emoji(":check: [green]Passed[/]  :cross: [red]Failed[/]  :warning: [yellow]Warning[/]"));
    con.print(replace_emoji(":coffee: Coffee  :beer: Beer  :pizza: Pizza  :tada: Party"));
    con.print(replace_emoji(":bug: Bug  :wrench: Fix  :sparkles: Feature  :memo: Docs"));
}

// ═══════════════════════════════════════════════════════════════════════
//  10. Markdown Rendering
// ═══════════════════════════════════════════════════════════════════════

static void demo_markdown(console &con) {
    section(con, " 10. Markdown Rendering ");

    const char *md = "# tapiru Library\n"
                     "\n"
                     "A **modern** C++23 library for *rich* terminal output.\n"
                     "\n"
                     "## Features\n"
                     "\n"
                     "- **Markup parser** with `[tag]` syntax\n"
                     "- *Widget system* with layout engine\n"
                     "- Live display with `async` rendering\n"
                     "\n"
                     "---\n"
                     "\n"
                     "> The best way to predict the future is to **invent** it.\n";

    con.print_widget(markdown_builder(md), 70);
}

// ═══════════════════════════════════════════════════════════════════════
//  11. Logging Handler
// ═══════════════════════════════════════════════════════════════════════

static void demo_logging(console &con) {
    section(con, " 11. Logging Handler ");

    log_handler logger(con);
    logger.show_timestamp(true);

    logger.trace("Initializing subsystems...");
    logger.debug("Config loaded from tapiru.toml");
    logger.info("Server started on port 8080");
    logger.warn("Disk usage at 85%");
    logger.error("Connection to database failed");
    logger.fatal("Out of memory — shutting down");
}

// ═══════════════════════════════════════════════════════════════════════
//  12. Progress Bar (static snapshot)
// ═══════════════════════════════════════════════════════════════════════

static void demo_progress(console &con) {
    section(con, " 12. Progress Bar ");

    auto t1 = std::make_shared<progress_task>("Downloading", 100);
    auto t2 = std::make_shared<progress_task>("Compiling", 200);
    auto t3 = std::make_shared<progress_task>("Linking", 50);

    t1->set_completed();
    t2->advance(140);
    t3->advance(10);

    con.print_widget(progress_builder()
                         .add_task(t1)
                         .add_task(t2)
                         .add_task(t3)
                         .bar_width(30)
                         .complete_char(U'\x2588')   // █
                         .remaining_char(U'\x2591'), // ░
                     70);
}

// ═══════════════════════════════════════════════════════════════════════
//  13. Spinner (static snapshot)
// ═══════════════════════════════════════════════════════════════════════

static void demo_spinner(console &con) {
    section(con, " 13. Spinner ");

    con.print("[dim]Spinner frames (dots):[/]");
    auto st1 = std::make_shared<spinner_state>();
    for (int i = 0; i < 10; ++i) {
        con.print_widget(spinner_builder(st1).message("Loading..."), 40);
    }

    con.newline();
    con.print("[dim]Spinner done state:[/]");
    auto st2 = std::make_shared<spinner_state>();
    st2->set_done();
    con.print_widget(spinner_builder(st2).message("Complete!").done_text("[green]OK[/]"), 40);
}

// ═══════════════════════════════════════════════════════════════════════
//  14. Gradient Panels
// ═══════════════════════════════════════════════════════════════════════

static void demo_gradient(console &con) {
    section(con, " 14. Gradient Panels ");

    panel_builder pb(text_builder("[bold white]Gradient background: cyan → magenta[/]"));
    pb.title("Gradient");
    pb.border(border_style::rounded);
    pb.background_gradient({color::from_rgb(0, 200, 200), color::from_rgb(200, 0, 200)});
    con.print_widget(std::move(pb), 50);

    con.newline();

    con.print_widget(
        rule_builder("Gradient Rule").gradient({color::from_rgb(255, 100, 0), color::from_rgb(0, 100, 255)}), 70);
}

// ═══════════════════════════════════════════════════════════════════════
//  15. Braille Charts
// ═══════════════════════════════════════════════════════════════════════

static void demo_charts(console &con) {
    section(con, " 15. Braille Charts ");

    // Line chart — sine wave
    con.print("[dim]Line chart (sine wave):[/]");
    std::vector<float> sine_data;
    for (int i = 0; i < 80; ++i) {
        sine_data.push_back(static_cast<float>(std::sin(i * 0.15) * 50 + 50));
    }
    con.print_widget(line_chart_builder(sine_data, 40, 6), 70);

    con.newline();

    // Bar chart
    con.print("[dim]Bar chart:[/]");
    con.print_widget(bar_chart_builder({3, 7, 2, 9, 5, 8, 1, 6}, 6).labels({"M", "T", "W", "T", "F", "S", "S", "A"}),
                     70);

    con.newline();

    // Scatter plot
    con.print("[dim]Scatter plot:[/]");
    std::vector<scatter_point> pts;
    for (int i = 0; i < 30; ++i) {
        float x = static_cast<float>(i);
        float y = x * 0.5f + static_cast<float>((i * 7 + 3) % 11);
        pts.push_back({x, y});
    }
    con.print_widget(scatter_builder(pts, 30, 5), 70);
}

// ═══════════════════════════════════════════════════════════════════════
//  16. Image to Character Art
// ═══════════════════════════════════════════════════════════════════════

static void demo_image(console &con) {
    section(con, " 16. Image to Character Art ");

    // Generate a 16x16 gradient image (red→blue horizontal, green→black vertical)
    std::vector<pixel_rgba> pixels;
    for (uint32_t y = 0; y < 16; ++y) {
        for (uint32_t x = 0; x < 16; ++x) {
            uint8_t r = static_cast<uint8_t>(x * 255 / 15);
            uint8_t g = static_cast<uint8_t>((15 - y) * 255 / 15);
            uint8_t b = static_cast<uint8_t>((15 - x) * 255 / 15);
            pixels.push_back({r, g, b, 255});
        }
    }
    con.print("[dim]16x16 gradient rendered as half-block art:[/]");
    con.print_widget(image_builder(pixels, 16, 16).target_width(16), 70);
}

// ═══════════════════════════════════════════════════════════════════════
//  17. Theme System
// ═══════════════════════════════════════════════════════════════════════

static void demo_theme(console &con) {
    section(con, " 17. Theme System ");

    auto th = theme::dark();
    con.print("[dim]Dark theme preset styles:[/]");

    auto show = [&](const char *name) {
        auto *s = th.lookup(name);
        if (!s) return;
        char buf[128];
        std::snprintf(buf, sizeof(buf), "  [#%02X%02X%02X]%s[/]  fg=(%u,%u,%u)", s->fg.r, s->fg.g, s->fg.b, name,
                      s->fg.r, s->fg.g, s->fg.b);
        con.print(buf);
    };

    show("danger");
    show("success");
    show("info");
    show("warning");
    show("accent");
    show("muted");
}

// ═══════════════════════════════════════════════════════════════════════
//  18. Structured Logging
// ═══════════════════════════════════════════════════════════════════════

static void demo_structured_logging(console &con) {
    section(con, " 18. Structured Logging ");

    log_handler logger(con);
    logger.show_timestamp(true);
    logger.set_module("http");

    logger.log_structured(log_level::info, "Request handled",
                          {{"method", "GET"}, {"path", "/api/users"}, {"status", "200"}});
    logger.log_structured(log_level::warn, "Slow query", {{"duration", "1200ms"}, {"table", "orders"}});

    logger.set_module("auth");
    logger.log_structured(log_level::error, "Login failed", {{"user", "admin"}, {"reason", "bad_password"}});
}

// ═══════════════════════════════════════════════════════════════════════
//  19. GFM Markdown
// ═══════════════════════════════════════════════════════════════════════

static void demo_gfm_markdown(console &con) {
    section(con, " 19. GFM Markdown ");

    const char *md = "## Task List\n"
                     "\n"
                     "- [x] Canvas optimization\n"
                     "- [x] Gradient engine\n"
                     "- [ ] Deploy to production\n"
                     "\n"
                     "## Code Block\n"
                     "\n"
                     "```cpp\n"
                     "int main() {\n"
                     "    return 0;\n"
                     "}\n"
                     "```\n"
                     "\n"
                     "## Table\n"
                     "\n"
                     "| Feature | Status |\n"
                     "|---------|--------|\n"
                     "| Charts  | Done   |\n"
                     "| Themes  | Done   |\n";

    con.print_widget(markdown_builder(md), 70);
}

// ═══════════════════════════════════════════════════════════════════════
//  20. Rows Layout
// ═══════════════════════════════════════════════════════════════════════

static void demo_rows(console &con) {
    section(con, " 20. Rows Layout ");

    rows_builder rb;
    rb.add(text_builder("[bold]Row 1:[/] Top item"), 0);
    rb.add(text_builder("[bold]Row 2:[/] Middle item (flex=1)"), 1);
    rb.add(text_builder("[bold]Row 3:[/] Bottom item"), 0);
    rb.gap(0);
    con.print_widget(std::move(rb), 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  21. Status Bar
// ═══════════════════════════════════════════════════════════════════════

static void demo_status_bar(console &con) {
    section(con, " 21. Status Bar ");

    con.print_widget(status_bar_builder()
                         .left("[bold] NORMAL [/]")
                         .center("main.cpp")
                         .right("Ln 42, Col 8")
                         .style_override(style{colors::bright_white, color::from_rgb(40, 40, 80)}),
                     70);
}

// ═══════════════════════════════════════════════════════════════════════
//  22. Menu Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_menu(console &con) {
    section(con, " 22. Menu Widget ");

    int cursor = 1;
    con.print_widget(menu_builder()
                         .add_item("New File", "Ctrl+N")
                         .add_item("Open File", "Ctrl+O")
                         .add_item("Save", "Ctrl+S")
                         .add_separator()
                         .add_item("Exit", "Ctrl+Q")
                         .cursor(&cursor)
                         .highlight_style(style{colors::bright_white, colors::blue, attr::bold})
                         .shortcut_style(style{colors::bright_black})
                         .shadow(),
                     30);
}

// ═══════════════════════════════════════════════════════════════════════
//  23. Table with Shadow
// ═══════════════════════════════════════════════════════════════════════

static void demo_table_shadow(console &con) {
    section(con, " 23. Table with Shadow ");

    table_builder tb;
    tb.add_column("Feature", {justify::left, 15, 25});
    tb.add_column("Status", {justify::center, 8, 12});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});
    tb.shadow();

    tb.add_row({"Rows layout", "[green]Done[/]"});
    tb.add_row({"Status bar", "[green]Done[/]"});
    tb.add_row({"Menu widget", "[green]Done[/]"});
    tb.add_row({"Popup widget", "[green]Done[/]"});
    tb.add_row({"Table shadow", "[green]Done[/]"});

    con.print_widget(tb, 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  24. Live Display (animated)
// ═══════════════════════════════════════════════════════════════════════

static void demo_live(console &con) {
    section(con, " 24. Live Display (animated) ");

    if (!terminal::is_tty()) {
        con.print("[yellow]Skipped: requires interactive terminal (TTY)[/]");
        con.print("[dim]Run this demo directly in Windows Terminal to see the live animation.[/]");
        return;
    }

    con.print("[dim]Running a 2-second live progress animation...[/]");
    con.newline();

    auto task = std::make_shared<progress_task>("Processing", 100);

    {
        live lv(con, 15, 70);
        lv.set(progress_builder().add_task(task).bar_width(40).complete_char('#').remaining_char('.'));

        for (int i = 0; i <= 100; i += 5) {
            task->set_current(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        task->set_completed();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    con.newline();
    con.print("[bold green]Live display finished![/]");
}

// ═══════════════════════════════════════════════════════════════════════
//  25. Element Pipe Composition
// ═══════════════════════════════════════════════════════════════════════

static void demo_element_pipe(console &con) {
    section(con, " 25. Element Pipe Composition ");

    con.print("[dim]Compose elements with decorator pipes: elem | border() | bold()[/]");
    con.newline();

    // Simple pipe: text | border | bold
    auto e1 = element(text_builder("Hello, pipes!")) | border() | bold();
    con.print_widget(e1, 30);

    con.newline();

    // Chained decorators: padding + border + color
    auto e2 = element(text_builder("Styled with pipes")) | padding(1, 2) | border(border_style::rounded) |
              fg_color(colors::bright_cyan);
    con.print_widget(e2, 40);

    con.newline();

    // Centered element
    auto e3 = element(text_builder("[bold]Centered![/]")) | border(border_style::heavy) | center();
    con.print_widget(e3, 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  26. Canvas Drawing (Braille / Block)
// ═══════════════════════════════════════════════════════════════════════

static void demo_canvas(console &con) {
    section(con, " 26. Canvas Drawing (Braille) ");

    // 26a. Simple shapes
    con.print("[dim]26a. Points, lines, and circles on a braille canvas:[/]");
    {
        auto cvs = make_canvas(60, 32, [](canvas_widget_builder &c) {
            // Border rectangle
            c.draw_rect(0, 0, 59, 31, colors::bright_black);

            // Diagonal lines
            c.draw_line(2, 2, 57, 29, colors::green);
            c.draw_line(57, 2, 2, 29, colors::red);

            // Circle in center
            c.draw_circle(30, 16, 12, colors::cyan);

            // Some points
            for (int i = 0; i < 60; i += 5) {
                c.draw_point(i, 0, colors::yellow);
                c.draw_point(i, 31, colors::yellow);
            }
        });
        con.print_widget(cvs, 70);
    }

    con.newline();

    // 26b. Sine wave with text overlay
    con.print("[dim]26b. Sine wave with text overlay:[/]");
    {
        auto cvs = make_canvas(80, 24, [](canvas_widget_builder &c) {
            // Draw axes
            c.draw_line(0, 12, 79, 12, colors::bright_black);
            c.draw_line(0, 0, 0, 23, colors::bright_black);

            // Sine wave
            for (int x = 0; x < 80; ++x) {
                int y = 12 + static_cast<int>(10.0 * std::sin(x * 0.15));
                if (y >= 0 && y < 24) c.draw_point(x, y, colors::bright_green);
            }

            // Text label
            c.draw_text(2, 1, "sin(x)", style{colors::bright_yellow, {}, attr::bold});
        });
        con.print_widget(cvs, 70);
    }

    con.newline();

    // 26c. Block-mode lines
    con.print("[dim]26c. Block-mode lines (half-block characters):[/]");
    {
        canvas_widget_builder cvs(40, 20);
        cvs.draw_block_line(0, 0, 39, 19, colors::magenta);
        cvs.draw_block_line(0, 19, 39, 0, colors::bright_blue);
        cvs.draw_block_line(20, 0, 20, 19, colors::bright_yellow);
        con.print_widget(std::move(cvs), 70);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  27. Gauge Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_gauge(console &con) {
    section(con, " 27. Gauge Widget ");

    // 27a. Horizontal gauges at various levels
    con.print("[dim]27a. Horizontal gauges at various progress levels:[/]");
    {
        rows_builder rb;

        auto add_gauge_row = [&](const char *label, float progress) {
            columns_builder row;
            row.add(text_builder(label));
            row.add(make_gauge(progress), 1);
            row.gap(1);
            rb.add(std::move(row));
        };

        add_gauge_row("[dim]  0%[/]", 0.0f);
        add_gauge_row("[dim] 25%[/]", 0.25f);
        add_gauge_row("[dim] 50%[/]", 0.50f);
        add_gauge_row("[dim] 75%[/]", 0.75f);
        add_gauge_row("[bold]100%[/]", 1.0f);

        rb.gap(0);
        con.print_widget(std::move(rb), 50);
    }

    con.newline();

    // 27b. Styled gauges
    con.print("[dim]27b. Custom-styled gauge (green fill, dim remaining):[/]");
    {
        style filled{colors::bright_green, {}, attr::bold};
        style remaining{colors::bright_black, {}, attr::dim};
        con.print_widget(make_gauge(0.65f, filled, remaining), 50);
    }

    con.newline();

    // 27c. Vertical gauge
    con.print("[dim]27c. Vertical gauge (60% filled):[/]");
    {
        columns_builder cols;
        cols.add(make_gauge_direction(0.3f, gauge_direction::vertical));
        cols.add(make_gauge_direction(0.6f, gauge_direction::vertical));
        cols.add(make_gauge_direction(0.9f, gauge_direction::vertical));
        cols.gap(2);
        con.print_widget(std::move(cols), 20);
    }

    con.newline();

    // 27d. Gauge in a dashboard panel
    con.print("[dim]27d. Gauge in a dashboard panel:[/]");
    {
        rows_builder inner;
        inner.add(text_builder("[bold]CPU Usage[/]"));
        inner.add(make_gauge(0.73f));
        inner.add(text_builder("[bold]Memory[/]"));
        inner.add(make_gauge(0.45f));
        inner.add(text_builder("[bold]Disk I/O[/]"));
        inner.add(make_gauge(0.12f));
        inner.gap(0);

        panel_builder pb(std::move(inner));
        pb.title("System Monitor");
        pb.border(border_style::rounded);
        con.print_widget(std::move(pb), 50);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  28. Paragraph Widget (Word Wrap)
// ═══════════════════════════════════════════════════════════════════════

static void demo_paragraph(console &con) {
    section(con, " 28. Paragraph Widget ");

    // 28a. Basic word-wrapping paragraph
    con.print("[dim]28a. Word-wrapped paragraph (narrow width):[/]");
    {
        auto para = make_paragraph("The quick brown fox jumps over the lazy dog. "
                                   "This sentence demonstrates automatic word wrapping in the tapiru "
                                   "paragraph widget. Long text is broken at word boundaries to fit "
                                   "within the available width.");
        panel_builder pb(std::move(para));
        pb.border(border_style::rounded);
        pb.title("Word Wrap");
        con.print_widget(std::move(pb), 45);
    }

    con.newline();

    // 28b. Justified paragraph
    con.print("[dim]28b. Justified paragraph (spaces distributed evenly):[/]");
    {
        auto para = make_paragraph_justify("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                                           "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                                           "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.");
        panel_builder pb(std::move(para));
        pb.border(border_style::rounded);
        pb.title("Justified");
        con.print_widget(std::move(pb), 50);
    }

    con.newline();

    // 28c. Paragraph with explicit newlines
    con.print("[dim]28c. Paragraph preserving explicit newlines:[/]");
    {
        auto para = make_paragraph("Line one: Introduction\n"
                                   "Line two: Details and explanation\n"
                                   "Line three: Conclusion");
        con.print_widget(para, 50);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  29. Color Downgrade
// ═══════════════════════════════════════════════════════════════════════

static void demo_color_downgrade(console &con) {
    section(con, " 29. Color Downgrade ");

    con.print("[dim]color::downgrade() converts colors to lower bit depths:[/]");
    con.newline();

    // Show RGB → 256 → 16 → none chain
    auto original = color::from_rgb(255, 100, 0); // orange
    auto to_256 = original.downgrade(2);
    auto to_16 = original.downgrade(1);
    auto to_none = original.downgrade(0);

    char buf[256];
    std::snprintf(buf, sizeof(buf), "[#FF6400]  RGB(255,100,0)[/]  → 256-idx=%u → 16-idx=%u → %s", to_256.r, to_16.r,
                  to_none.is_default() ? "default" : "???");
    con.print(buf);

    // Demonstrate with a table
    table_builder tb;
    tb.add_column("Original", {justify::left, 18, 22});
    tb.add_column("→ 256", {justify::center, 10, 14});
    tb.add_column("→ 16", {justify::center, 10, 14});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});

    auto show_downgrade = [&](const char *name, color c) {
        auto d256 = c.downgrade(2);
        auto d16 = c.downgrade(1);
        char c256[32], c16[32];
        std::snprintf(c256, sizeof(c256), "idx %u", d256.r);
        std::snprintf(c16, sizeof(c16), "idx %u", d16.r);
        tb.add_row({name, c256, c16});
    };

    show_downgrade("[red]Pure Red[/]", color::from_rgb(255, 0, 0));
    show_downgrade("[green]Pure Green[/]", color::from_rgb(0, 255, 0));
    show_downgrade("[blue]Pure Blue[/]", color::from_rgb(0, 0, 255));
    show_downgrade("[#FF8000]Orange[/]", color::from_rgb(255, 128, 0));
    show_downgrade("[dim]Gray(128)[/]", color::from_rgb(128, 128, 128));
    show_downgrade("[bright_white]White[/]", color::from_rgb(255, 255, 255));

    con.print_widget(tb, 55);
}

// ═══════════════════════════════════════════════════════════════════════
//  30. Double Underline & New Attributes
// ═══════════════════════════════════════════════════════════════════════

static void demo_double_underline(console &con) {
    section(con, " 30. Double Underline & Attributes ");

    con.print("[dim]attr::double_underline (SGR 21) — new in Stage 7:[/]");
    con.newline();

    // Show all attribute combinations in a table
    table_builder tb;
    tb.add_column("Attribute", {justify::left, 20, 30});
    tb.add_column("Bit Value", {justify::right, 10, 15});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});

    tb.add_row({"[bold]bold[/]", "0x0001"});
    tb.add_row({"[dim]dim[/]", "0x0002"});
    tb.add_row({"[italic]italic[/]", "0x0004"});
    tb.add_row({"[underline]underline[/]", "0x0008"});
    tb.add_row({"[blink]blink[/]", "0x0010"});
    tb.add_row({"[reverse]reverse[/]", "0x0020"});
    tb.add_row({"[hidden]hidden[/]", "0x0040"});
    tb.add_row({"[strike]strikethrough[/]", "0x0080"});
    tb.add_row({"double_underline (new!)", "0x0100"});

    con.print_widget(tb, 50);

    con.newline();
    con.print("[dim]Combining double_underline with other attrs:[/]");

    // Show combined styles
    style du_style;
    du_style.attrs = attr::double_underline | attr::bold;
    du_style.fg = colors::bright_cyan;
    auto tb2 = text_builder("[bold]bold + double_underline[/]");
    tb2.style_override(du_style);
    con.print_widget(std::move(tb2), 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    console con;

    con.newline();
    con.print_widget(rule_builder(" tapiru — Comprehensive Feature Demo ")
                         .rule_style(style{colors::bright_yellow, {}, attr::bold})
                         .character(U'\x2550'), // ═
                     70);

    demo_markup(con);
    demo_constexpr_markup(con);
    demo_text_widget(con);
    demo_rule(con);
    demo_panel(con);
    demo_padding(con);
    demo_columns(con);
    demo_table(con);
    demo_emoji(con);
    demo_markdown(con);
    demo_logging(con);
    demo_progress(con);
    demo_spinner(con);
    demo_gradient(con);
    demo_charts(con);
    demo_image(con);
    demo_theme(con);
    demo_structured_logging(con);
    demo_gfm_markdown(con);
    demo_rows(con);
    demo_status_bar(con);
    demo_menu(con);
    demo_table_shadow(con);
    demo_live(con);
    demo_element_pipe(con);
    demo_canvas(con);
    demo_gauge(con);
    demo_paragraph(con);
    demo_color_downgrade(con);
    demo_double_underline(con);

    con.newline();
    con.print_widget(rule_builder(" Demo Complete! ")
                         .rule_style(style{colors::bright_green, {}, attr::bold})
                         .character(U'\x2550'), // ═
                     70);
    con.newline();

    return 0;
}
