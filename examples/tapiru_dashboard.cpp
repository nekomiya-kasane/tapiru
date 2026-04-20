/**
 * @file tapiru_dashboard.cpp
 * @brief Dashboard demo showcasing all tapiru features with performance metrics.
 *
 * Build:
 *   cmake --build build --target tapiru_dashboard --config Release
 * Run:
 *   .\build\bin\Release\tapiru_dashboard.exe
 */

#include "tapiru/core/console.h"
#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/core/gradient.h"
#include "tapiru/core/logging.h"
#include "tapiru/core/style.h"
#include "tapiru/core/theme.h"
#include "tapiru/text/constexpr_markup.h"
#include "tapiru/text/emoji.h"
#include "tapiru/text/markdown.h"
#include "tapiru/text/markup.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/canvas_widget.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/gauge.h"
#include "tapiru/widgets/image.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/paragraph.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/spinner.h"
#include "tapiru/widgets/status_bar.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

using namespace tapiru;

static constexpr int W = 78;

// ── Micro-benchmark helper ──────────────────────────────────────────────

struct bench_result {
    const char *name;
    double us; // microseconds
    int ops;   // operations performed
};

template <typename F> static bench_result bench(const char *name, int ops, F &&fn) {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ops; ++i) fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    return {name, us, ops};
}

static void section(console &con, const char *title) {
    con.newline();
    con.print_widget(rule_builder(title).rule_style(style{colors::bright_cyan, {}, attr::bold}), W);
    con.newline();
}

// ═══════════════════════════════════════════════════════════════════════
//  1. Header
// ═══════════════════════════════════════════════════════════════════════

static void show_header(console &con) {
    con.newline();
    con.print_widget(rule_builder(" tapiru — Feature & Performance Dashboard ")
                         .rule_style(style{colors::bright_yellow, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    rows_builder info;
    info.add(text_builder("[bold bright_white]tapiru[/] [dim]v1.0[/]  —  Terminal & TUI Library for C++23"));
    info.add(text_builder("[dim]Rich markup, composable widgets, live display, charts, visual effects[/]"));
    info.gap(0);

    panel_builder pb(std::move(info));
    pb.title("About");
    pb.border(border_style::rounded);
    pb.border_style_override(style{colors::bright_cyan});
    con.print_widget(std::move(pb), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  2. Feature Matrix
// ═══════════════════════════════════════════════════════════════════════

static void show_feature_matrix(console &con) {
    section(con, " Feature Matrix ");

    table_builder tb;
    tb.add_column("Category", {justify::left, 16, 22});
    tb.add_column("Feature", {justify::left, 24, 32});
    tb.add_column("Status", {justify::center, 8, 12});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});
    tb.shadow();

    auto ok = "[green]Ready[/]";

    tb.add_row({"Core", "Rich Markup Parser", ok});
    tb.add_row({"Core", "Constexpr Markup", ok});
    tb.add_row({"Core", "Console (print/widget)", ok});
    tb.add_row({"Core", "Live Display Engine", ok});
    tb.add_row({"Core", "Theme System", ok});
    tb.add_row({"Core", "Logging Handler", ok});
    tb.add_row({"Widgets", "Panel / Table / Columns", ok});
    tb.add_row({"Widgets", "Rows / Rule / Spacer", ok});
    tb.add_row({"Widgets", "Menu / Popup / Overlay", ok});
    tb.add_row({"Widgets", "Progress / Spinner / Gauge", ok});
    tb.add_row({"Widgets", "Status Bar / Paragraph", ok});
    tb.add_row({"Interactive", "Button / Checkbox / Radio", ok});
    tb.add_row({"Interactive", "Slider / Text Input", ok});
    tb.add_row({"Interactive", "Selectable List", ok});
    tb.add_row({"Data Viz", "Line Chart (braille)", ok});
    tb.add_row({"Data Viz", "Bar Chart (block)", ok});
    tb.add_row({"Data Viz", "Scatter Plot", ok});
    tb.add_row({"Data Viz", "Canvas (braille/block)", ok});
    tb.add_row({"Data Viz", "Image (half-block pixel)", ok});
    tb.add_row({"VFX", "Shadow / Glow / Gradient", ok});
    tb.add_row({"VFX", "Shaders (scanline/shimmer)", ok});
    tb.add_row({"VFX", "Animation / Easing / Tween", ok});
    tb.add_row({"Text", "Emoji Shortcodes", ok});
    tb.add_row({"Text", "Markdown Renderer", ok});
    tb.add_row({"Text", "Element Pipe Composition", ok});
    tb.add_row({"Misc", "tqdm Progress Iterator", ok});
    tb.add_row({"Misc", "Cross-platform (Win/POSIX)", ok});
    tb.add_row({"Misc", "Color Depth Auto-detect", ok});

    con.print_widget(tb, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  3. Performance Benchmarks
// ═══════════════════════════════════════════════════════════════════════

static void show_perf_benchmarks(console &con) {
    section(con, " Performance Benchmarks ");

    std::vector<bench_result> results;

    // Markup parsing
    results.push_back(bench("Markup parse (short)", 100000, [] {
        volatile auto spans = parse_markup("[bold red]Hello[/] [green]World[/]");
        (void)spans;
    }));

    // Constexpr markup (compile-time, but measure the runtime path for comparison)
    results.push_back(bench("Markup parse (long)", 50000, [] {
        volatile auto spans = parse_markup("[bold]Title[/] [dim]subtitle[/] [italic cyan on_#112233]styled[/] "
                                           "[#FF8800]rgb[/] [underline strike]decorated[/] plain text");
        (void)spans;
    }));

    // Emoji replacement
    results.push_back(bench("Emoji replace", 50000, [] {
        volatile auto s = replace_emoji(":rocket: Launch :fire: Hot :star: Star");
        (void)s;
    }));

    // Text builder construction
    results.push_back(bench("Text builder create", 100000, [] {
        volatile auto tb = text_builder("[bold green]Hello[/], [italic]tapiru[/]!");
        (void)tb;
    }));

    // Table builder construction
    results.push_back(bench("Table build (5 rows)", 10000, [] {
        table_builder t;
        t.add_column("A", {justify::left, 10, 20});
        t.add_column("B", {justify::center, 10, 20});
        t.border(border_style::rounded);
        t.add_row({"r1a", "r1b"});
        t.add_row({"r2a", "r2b"});
        t.add_row({"r3a", "r3b"});
        t.add_row({"r4a", "r4b"});
        t.add_row({"r5a", "r5b"});
    }));

    // Panel builder construction
    results.push_back(bench("Panel build", 50000, [] {
        panel_builder pb(text_builder("Content"));
        pb.title("Title");
        pb.border(border_style::rounded);
    }));

    // Display results as table
    table_builder tb;
    tb.add_column("Benchmark", {justify::left, 26, 34});
    tb.add_column("Ops", {justify::right, 8, 12});
    tb.add_column("Total (ms)", {justify::right, 10, 14});
    tb.add_column("Per-op (us)", {justify::right, 10, 14});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_yellow, {}, attr::bold});

    for (auto &r : results) {
        char ops_buf[32], total_buf[32], per_buf[32];
        std::snprintf(ops_buf, sizeof(ops_buf), "%d", r.ops);
        std::snprintf(total_buf, sizeof(total_buf), "%.1f", r.us / 1000.0);
        std::snprintf(per_buf, sizeof(per_buf), "%.2f", r.us / r.ops);
        tb.add_row({r.name, ops_buf, total_buf, per_buf});
    }
    con.print_widget(tb, W);

    // Bar chart of per-op times
    con.newline();
    con.print("[dim]Per-operation latency (us):[/]");
    std::vector<float> bar_data;
    std::vector<std::string> bar_labels;
    for (auto &r : results) {
        bar_data.push_back(static_cast<float>(r.us / r.ops));
        // Short label: first word
        std::string label(r.name);
        auto sp = label.find(' ');
        if (sp != std::string::npos) label = label.substr(0, sp);
        bar_labels.push_back(label);
    }
    con.print_widget(bar_chart_builder(bar_data, 6).labels(bar_labels).style_override(style{colors::bright_green}), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  4. Widget Showcase (live examples)
// ═══════════════════════════════════════════════════════════════════════

static void show_widget_showcase(console &con) {
    section(con, " Widget Showcase ");

    // Markup styles
    con.print("[bold]Bold[/] [italic]Italic[/] [underline]Underline[/] [strike]Strike[/] "
              "[red]Red[/] [green]Green[/] [blue]Blue[/] [#FF8800]RGB Orange[/]");
    con.newline();

    // Columns layout
    columns_builder cols;
    {
        rows_builder left;
        left.add(text_builder("[bold]Gauges[/]"));
        left.add(text_builder("[dim]CPU[/]"));
        left.add(make_gauge(0.73f));
        left.add(text_builder("[dim]RAM[/]"));
        left.add(make_gauge(0.45f));
        left.add(text_builder("[dim]Disk[/]"));
        left.add(make_gauge(0.88f));
        left.gap(0);
        panel_builder lp(std::move(left));
        lp.title("System");
        lp.border(border_style::rounded);
        cols.add(std::move(lp), 1);
    }
    {
        auto t1 = std::make_shared<progress_task>("Build", 100);
        auto t2 = std::make_shared<progress_task>("Test", 200);
        auto t3 = std::make_shared<progress_task>("Deploy", 50);
        t1->set_completed();
        t2->advance(160);
        t3->advance(15);

        rows_builder right;
        right.add(text_builder("[bold]Progress[/]"));
        right.add(progress_builder().add_task(t1).add_task(t2).add_task(t3).bar_width(20));
        right.gap(0);
        panel_builder rp(std::move(right));
        rp.title("Tasks");
        rp.border(border_style::rounded);
        cols.add(std::move(rp), 1);
    }
    cols.gap(1);
    con.print_widget(std::move(cols), W);

    con.newline();

    // Charts side by side
    columns_builder chart_cols;
    {
        std::vector<float> sine;
        for (int i = 0; i < 40; ++i) sine.push_back(static_cast<float>(std::sin(i * 0.2) * 40 + 50));
        panel_builder cp(line_chart_builder(sine, 30, 5));
        cp.title("Line Chart");
        cp.border(border_style::rounded);
        chart_cols.add(std::move(cp), 1);
    }
    {
        panel_builder bp(bar_chart_builder({8, 15, 6, 22, 12, 18, 9}, 5)
                             .labels({"M", "T", "W", "T", "F", "S", "S"})
                             .style_override(style{colors::bright_cyan}));
        bp.title("Bar Chart");
        bp.border(border_style::rounded);
        chart_cols.add(std::move(bp), 1);
    }
    chart_cols.gap(1);
    con.print_widget(std::move(chart_cols), W);

    con.newline();

    // Emoji + Markdown preview
    con.print(replace_emoji(":rocket: [bold]Emoji[/]  :fire: Hot  :star: Star  :check: Pass  :cross: Fail"));
    con.newline();

    // Status bar
    con.print_widget(status_bar_builder()
                         .left("[bold] tapiru [/]")
                         .center("dashboard.cpp")
                         .right("28 features | 6 benchmarks")
                         .style_override(style{colors::bright_white, color::from_rgb(30, 30, 70)}),
                     W);
}

// ═══════════════════════════════════════════════════════════════════════
//  5. Canvas Demo
// ═══════════════════════════════════════════════════════════════════════

static void show_canvas_demo(console &con) {
    section(con, " Canvas Drawing ");

    auto cvs = make_canvas(60, 24, [](canvas_widget_builder &c) {
        c.draw_rect(0, 0, 59, 23, colors::bright_black);
        for (int x = 0; x < 60; ++x) {
            int y = 12 + static_cast<int>(8.0 * std::sin(x * 0.2));
            if (y >= 0 && y < 24) c.draw_point(x, y, colors::bright_green);
        }
        for (int x = 0; x < 60; ++x) {
            int y = 12 + static_cast<int>(6.0 * std::cos(x * 0.15));
            if (y >= 0 && y < 24) c.draw_point(x, y, colors::bright_cyan);
        }
        c.draw_text(2, 1, "sin(x) + cos(x)", style{colors::bright_yellow, {}, attr::bold});
    });
    con.print_widget(cvs, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    console con;

    show_header(con);
    show_feature_matrix(con);
    show_perf_benchmarks(con);
    show_widget_showcase(con);
    show_canvas_demo(con);

    con.newline();
    con.print_widget(rule_builder(" tapiru Dashboard Complete ")
                         .rule_style(style{colors::bright_green, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    return 0;
}
