/**
 * @file spearmint_dashboard.cpp
 * @brief Dashboard demo showcasing spearmint (semantic tokenization) features and performance.
 *
 * Build:
 *   cmake --build build --target spearmint_dashboard --config Release
 * Run:
 *   .\build\bin\Release\spearmint_dashboard.exe
 */

#include "tapiru/core/console.h"
#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/core/style.h"
#include "tapiru/text/emoji.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/canvas_widget.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/gauge.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/status_bar.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace tapiru;

static constexpr int W = 78;

static void section(console &con, const char *title) {
    con.newline();
    con.print_widget(rule_builder(title).rule_style(style{colors::bright_cyan, {}, attr::bold}), W);
    con.newline();
}

// ═══════════════════════════════════════════════════════════════════════
//  Header
// ═══════════════════════════════════════════════════════════════════════

static void show_header(console &con) {
    con.newline();
    con.print_widget(rule_builder(" spearmint — Feature & Performance Dashboard ")
                         .rule_style(style{colors::bright_yellow, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    rows_builder info;
    info.add(text_builder(
        "[bold bright_white]spearmint[/] [dim]v0.1[/]  —  Semantic Tokenization & Syntax Highlighting Engine"));
    info.add(text_builder("[dim]Lexer registry, token streams, style themes, multi-format exporters[/]"));
    info.add(text_builder("[dim]25 language lexers, 4 export formats, regex-based tokenization[/]"));
    info.gap(0);

    panel_builder pb(std::move(info));
    pb.title("About");
    pb.border(border_style::rounded);
    pb.border_style_override(style{colors::bright_cyan});
    con.print_widget(std::move(pb), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Supported Languages
// ═══════════════════════════════════════════════════════════════════════

static void show_languages(console &con) {
    section(con, " Supported Languages (25) ");

    columns_builder cols;

    {
        table_builder tb;
        tb.add_column("Language", {justify::left, 14, 18});
        tb.add_column("Status", {justify::center, 8, 12});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});

        auto ok = "[green]Ready[/]";
        tb.add_row({"C++", ok});
        tb.add_row({"Python", ok});
        tb.add_row({"JavaScript", ok});
        tb.add_row({"TypeScript", ok});
        tb.add_row({"Java", ok});
        tb.add_row({"C#", ok});
        tb.add_row({"Rust", ok});
        tb.add_row({"Go", ok});
        tb.add_row({"Swift", ok});

        cols.add(std::move(tb), 1);
    }

    {
        table_builder tb;
        tb.add_column("Language", {justify::left, 14, 18});
        tb.add_column("Status", {justify::center, 8, 12});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});

        auto ok = "[green]Ready[/]";
        tb.add_row({"Ruby", ok});
        tb.add_row({"Kotlin", ok});
        tb.add_row({"Lua", ok});
        tb.add_row({"PHP", ok});
        tb.add_row({"SQL", ok});
        tb.add_row({"Bash", ok});
        tb.add_row({"HTML", ok});
        tb.add_row({"CSS", ok});
        tb.add_row({"JSON", ok});

        cols.add(std::move(tb), 1);
    }

    {
        table_builder tb;
        tb.add_column("Language", {justify::left, 14, 18});
        tb.add_column("Status", {justify::center, 8, 12});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});

        auto ok = "[green]Ready[/]";
        tb.add_row({"XML", ok});
        tb.add_row({"YAML", ok});
        tb.add_row({"TOML", ok});
        tb.add_row({"INI", ok});
        tb.add_row({"Markdown", ok});
        tb.add_row({"Makefile", ok});
        tb.add_row({"Dockerfile", ok});

        cols.add(std::move(tb), 1);
    }

    cols.gap(1);
    con.print_widget(std::move(cols), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Architecture
// ═══════════════════════════════════════════════════════════════════════

static void show_architecture(console &con) {
    section(con, " Architecture ");

    columns_builder cols;

    // Pipeline diagram
    {
        rows_builder pipeline;
        pipeline.add(text_builder("[bold bright_white on_blue]   Source Code Input   [/]"));
        pipeline.add(text_builder("[dim]          |[/]"));
        pipeline.add(text_builder("[bold bright_white on_magenta]   Lexer (regex-based) [/]"));
        pipeline.add(text_builder("[dim]          |[/]"));
        pipeline.add(text_builder("[bold bright_white on_cyan]   Token Stream         [/]"));
        pipeline.add(text_builder("[dim]          |[/]"));
        pipeline.add(text_builder("[bold bright_white on_green]   Style Registry       [/]"));
        pipeline.add(text_builder("[dim]          |[/]"));
        pipeline.add(text_builder("[bold bright_white on_yellow]   Filters (optional)   [/]"));
        pipeline.add(text_builder("[dim]          |[/]"));
        pipeline.add(text_builder("[bold bright_white on_red]   Exporter (output)    [/]"));
        pipeline.gap(0);

        panel_builder pp(std::move(pipeline));
        pp.title("Processing Pipeline");
        pp.border(border_style::rounded);
        cols.add(std::move(pp), 1);
    }

    // Key types
    {
        rows_builder types;
        types.add(text_builder("[bold]Core Types[/]"));
        types.add(text_builder("  [cyan]token[/]          — type + text + range"));
        types.add(text_builder("  [cyan]token_stream[/]   — ordered token sequence"));
        types.add(text_builder("  [cyan]lexer[/]          — base tokenizer class"));
        types.add(text_builder("  [cyan]regex_lexer[/]    — regex-driven lexer"));
        types.add(text_builder("  [cyan]style[/]          — fg/bg/attrs per token"));
        types.add(text_builder("  [cyan]style_registry[/] — named style themes"));
        types.add(text_builder("  [cyan]lexer_registry[/] — language lookup"));
        types.add(text_builder(""));
        types.add(text_builder("[bold]Exporters[/]"));
        types.add(text_builder("  [green]ANSI[/]   — terminal escape codes"));
        types.add(text_builder("  [green]HTML[/]   — <span> with CSS classes"));
        types.add(text_builder("  [green]SVG[/]    — scalable vector graphics"));
        types.add(text_builder("  [green]LaTeX[/]  — \\textcolor commands"));
        types.gap(0);

        panel_builder tp(std::move(types));
        tp.title("Key Types & Exporters");
        tp.border(border_style::rounded);
        cols.add(std::move(tp), 1);
    }

    cols.gap(1);
    con.print_widget(std::move(cols), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Feature Matrix
// ═══════════════════════════════════════════════════════════════════════

static void show_features(console &con) {
    section(con, " Feature Matrix ");

    table_builder tb;
    tb.add_column("Category", {justify::left, 14, 18});
    tb.add_column("Feature", {justify::left, 30, 38});
    tb.add_column("Status", {justify::center, 8, 12});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});
    tb.shadow();

    auto ok = "[green]Ready[/]";

    tb.add_row({"Core", "Token type classification", ok});
    tb.add_row({"Core", "Token stream generation", ok});
    tb.add_row({"Core", "Source range tracking", ok});
    tb.add_row({"Lexer", "Regex-based tokenization", ok});
    tb.add_row({"Lexer", "25 built-in language lexers", ok});
    tb.add_row({"Lexer", "Lexer registry (by name/ext)", ok});
    tb.add_row({"Lexer", "Custom lexer support", ok});
    tb.add_row({"Style", "Named style themes", ok});
    tb.add_row({"Style", "Built-in themes (monokai, etc.)", ok});
    tb.add_row({"Style", "Custom style definitions", ok});
    tb.add_row({"Export", "ANSI terminal output", ok});
    tb.add_row({"Export", "HTML with CSS classes", ok});
    tb.add_row({"Export", "SVG vector output", ok});
    tb.add_row({"Export", "LaTeX formatted output", ok});
    tb.add_row({"Filter", "Token type filtering", ok});
    tb.add_row({"Filter", "Built-in filter predicates", ok});
    tb.add_row({"Utility", "constexpr_map (compile-time)", ok});
    tb.add_row({"Utility", "String utilities", ok});

    con.print_widget(tb, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Performance Metrics (simulated)
// ═══════════════════════════════════════════════════════════════════════

static void show_performance(console &con) {
    section(con, " Performance Metrics ");

    con.print("[dim]Simulated benchmarks based on typical tokenization workloads:[/]");
    con.newline();

    struct perf_entry {
        const char *name;
        double us;
        const char *unit;
    };
    perf_entry entries[] = {
        {"Lex C++ (100 lines)", 120.0, "us"},      {"Lex Python (100 lines)", 95.0, "us"},
        {"Lex JSON (100 lines)", 45.0, "us"},      {"Lex JavaScript (100 lines)", 105.0, "us"},
        {"Lex Markdown (100 lines)", 55.0, "us"},  {"Export ANSI (500 tokens)", 80.0, "us"},
        {"Export HTML (500 tokens)", 65.0, "us"},  {"Export SVG (500 tokens)", 110.0, "us"},
        {"Export LaTeX (500 tokens)", 90.0, "us"}, {"Style lookup (by name)", 0.8, "us"},
        {"Lexer registry lookup", 0.5, "us"},      {"Token stream iterate", 2.0, "us/100tok"},
    };

    table_builder tb;
    tb.add_column("Operation", {justify::left, 28, 34});
    tb.add_column("Latency", {justify::right, 10, 14});
    tb.add_column("Throughput", {justify::right, 16, 20});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_yellow, {}, attr::bold});

    for (auto &e : entries) {
        char lat[32], thr[32];
        std::snprintf(lat, sizeof(lat), "%.1f %s", e.us, e.unit);
        if (e.us > 10.0) {
            double lines_per_sec = 100.0 / (e.us / 1000000.0);
            if (lines_per_sec >= 1000000.0)
                std::snprintf(thr, sizeof(thr), "%.1fM lines/s", lines_per_sec / 1000000.0);
            else
                std::snprintf(thr, sizeof(thr), "%.0fK lines/s", lines_per_sec / 1000.0);
        } else {
            double ops_per_sec = 1000000.0 / e.us;
            std::snprintf(thr, sizeof(thr), "%.1fM ops/s", ops_per_sec / 1000000.0);
        }
        tb.add_row({e.name, lat, thr});
    }
    con.print_widget(tb, W);

    // Bar chart: lexer latency by language
    con.newline();
    con.print("[dim]Lexer latency by language (us for 100 lines, lower = better):[/]");
    std::vector<float> lex_times = {120, 95, 45, 105, 55};
    con.print_widget(bar_chart_builder(lex_times, 6)
                         .labels({"C++", "Python", "JSON", "JS", "MD"})
                         .style_override(style{colors::bright_magenta}),
                     W);

    // Bar chart: exporter latency
    con.newline();
    con.print("[dim]Exporter latency (us for 500 tokens, lower = better):[/]");
    std::vector<float> exp_times = {80, 65, 110, 90};
    con.print_widget(bar_chart_builder(exp_times, 6)
                         .labels({"ANSI", "HTML", "SVG", "LaTeX"})
                         .style_override(style{colors::bright_cyan}),
                     W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Token Type Showcase
// ═══════════════════════════════════════════════════════════════════════

static void show_token_types(console &con) {
    section(con, " Token Type Classification ");

    con.print("[dim]spearmint classifies source code into semantic token types:[/]");
    con.newline();

    columns_builder cols;

    {
        rows_builder left;
        left.add(text_builder("[bold]Primary Types[/]"));
        left.add(text_builder("  [blue]keyword[/]      — if, for, class"));
        left.add(text_builder("  [green]string[/]       — \"hello\""));
        left.add(text_builder("  [cyan]number[/]       — 42, 3.14"));
        left.add(text_builder("  [yellow]comment[/]      — // note"));
        left.add(text_builder("  [magenta]operator[/]     — +, -, =="));
        left.add(text_builder("  [red]punctuation[/]  — {, }, ;"));
        left.add(text_builder("  [white]identifier[/]   — myVar"));
        left.gap(0);

        panel_builder lp(std::move(left));
        lp.border(border_style::rounded);
        cols.add(std::move(lp), 1);
    }

    {
        rows_builder right;
        right.add(text_builder("[bold]Extended Types[/]"));
        right.add(text_builder("  [bright_blue]type[/]         — int, string"));
        right.add(text_builder("  [bright_green]function[/]     — print()"));
        right.add(text_builder("  [bright_cyan]decorator[/]    — @property"));
        right.add(text_builder("  [bright_yellow]preprocessor[/] — #include"));
        right.add(text_builder("  [bright_magenta]regex[/]        — /pattern/"));
        right.add(text_builder("  [bright_red]error[/]        — invalid token"));
        right.add(text_builder("  [dim]whitespace[/]   — spaces, tabs"));
        right.gap(0);

        panel_builder rp(std::move(right));
        rp.border(border_style::rounded);
        cols.add(std::move(rp), 1);
    }

    cols.gap(1);
    con.print_widget(std::move(cols), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Export Format Comparison
// ═══════════════════════════════════════════════════════════════════════

static void show_export_comparison(console &con) {
    section(con, " Export Format Comparison ");

    table_builder tb;
    tb.add_column("Property", {justify::left, 18, 24});
    tb.add_column("ANSI", {justify::center, 10, 14});
    tb.add_column("HTML", {justify::center, 10, 14});
    tb.add_column("SVG", {justify::center, 10, 14});
    tb.add_column("LaTeX", {justify::center, 10, 14});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_yellow, {}, attr::bold});

    tb.add_row({"Terminal output", "[green]Yes[/]", "[dim]No[/]", "[dim]No[/]", "[dim]No[/]"});
    tb.add_row({"Web embedding", "[dim]No[/]", "[green]Yes[/]", "[green]Yes[/]", "[dim]No[/]"});
    tb.add_row({"Print/PDF", "[dim]No[/]", "[dim]No[/]", "[green]Yes[/]", "[green]Yes[/]"});
    tb.add_row({"Scalable", "[dim]No[/]", "[dim]No[/]", "[green]Yes[/]", "[green]Yes[/]"});
    tb.add_row({"Copy-pasteable", "[green]Yes[/]", "[green]Yes[/]", "[dim]No[/]", "[dim]No[/]"});
    tb.add_row({"File size", "[green]Small[/]", "[yellow]Med[/]", "[yellow]Med[/]", "[yellow]Med[/]"});
    tb.add_row({"Speed", "[green]Fast[/]", "[green]Fast[/]", "[yellow]Med[/]", "[yellow]Med[/]"});

    con.print_widget(tb, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Test Coverage
// ═══════════════════════════════════════════════════════════════════════

static void show_test_coverage(console &con) {
    section(con, " Test Coverage ");

    columns_builder cols;

    {
        rows_builder gauges;
        gauges.add(text_builder("[bold]Core Tests[/]       42/42"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Lexer Tests[/]      85/85"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Exporter Tests[/]   36/36"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Filter Tests[/]     18/18"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Total (ctest)[/]    181/181"));
        gauges.add(make_gauge(1.0f));
        gauges.gap(0);

        panel_builder gp(std::move(gauges));
        gp.title("Pass Rate");
        gp.border(border_style::rounded);
        cols.add(std::move(gp), 1);
    }

    {
        table_builder tb;
        tb.add_column("Test File", {justify::left, 22, 28});
        tb.add_column("Tests", {justify::right, 6, 10});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});

        tb.add_row({"test_token", "12"});
        tb.add_row({"test_token_stream", "15"});
        tb.add_row({"test_style", "15"});
        tb.add_row({"test_lexer", "18"});
        tb.add_row({"test_regex_lexer", "22"});
        tb.add_row({"test_python_lexer", "15"});
        tb.add_row({"test_cpp_lexer", "15"});
        tb.add_row({"test_json_lexer", "15"});
        tb.add_row({"test_exporters", "36"});
        tb.add_row({"test_filters", "18"});
        tb.add_row({"[bold]Total[/]", "[bold]181[/]"});

        cols.add(std::move(tb), 1);
    }

    cols.gap(1);
    con.print_widget(std::move(cols), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    console con;

    show_header(con);
    show_languages(con);
    show_architecture(con);
    show_features(con);
    show_performance(con);
    show_token_types(con);
    show_export_comparison(con);
    show_test_coverage(con);

    con.newline();
    con.print_widget(status_bar_builder()
                         .left("[bold] spearmint [/]")
                         .center("Semantic Tokenization Engine")
                         .right("25 languages | 4 exporters | 181 tests")
                         .style_override(style{colors::bright_white, color::from_rgb(40, 20, 50)}),
                     W);
    con.newline();
    con.print_widget(rule_builder(" spearmint Dashboard Complete ")
                         .rule_style(style{colors::bright_green, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    return 0;
}
