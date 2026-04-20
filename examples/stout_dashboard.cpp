/**
 * @file stout_dashboard.cpp
 * @brief Lightweight stout overview. The full stout vs Win32 dashboard with
 *        real benchmarks lives in stout/demos/stout_dashboard.cpp.
 *
 * Build the full version from the stout project:
 *   cd stout && cmake -B build -DSTOUT_BUILD_DASHBOARD=ON && cmake --build build --target stout_dashboard
 */

#include <cstdio>
#include <string>
#include <vector>

#include "tapiru/core/console.h"
#include "tapiru/core/style.h"
#include "tapiru/core/element.h"
#include "tapiru/core/decorator.h"
#include "tapiru/text/emoji.h"

#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/gauge.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/status_bar.h"
#include "tapiru/widgets/canvas_widget.h"

using namespace tapiru;

static constexpr int W = 78;

static void section(console& con, const char* title) {
    con.newline();
    con.print_widget(rule_builder(title)
        .rule_style(style{colors::bright_cyan, {}, attr::bold}), W);
    con.newline();
}

// ═══════════════════════════════════════════════════════════════════════
//  Header
// ═══════════════════════════════════════════════════════════════════════

static void show_header(console& con) {
    con.newline();
    con.print_widget(
        rule_builder(" stout — Feature & Performance Dashboard ")
            .rule_style(style{colors::bright_yellow, {}, attr::bold})
            .character(U'\x2550'), W);
    con.newline();

    rows_builder info;
    info.add(text_builder("[bold bright_white]stout[/] [dim]v1.0[/]  —  Compound File Binary (CFB) Library for C++23"));
    info.add(text_builder("[dim]Read/write Microsoft Structured Storage files (.doc, .xls, .msg, .msi)[/]"));
    info.add(text_builder("[dim]Full CFB v3/v4 support, property sets, Win32 IStorage/IStream interop[/]"));
    info.gap(0);

    panel_builder pb(std::move(info));
    pb.title("About");
    pb.border(border_style::rounded);
    pb.border_style_override(style{colors::bright_cyan});
    con.print_widget(std::move(pb), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Feature Matrix
// ═══════════════════════════════════════════════════════════════════════

static void show_features(console& con) {
    section(con, " Feature Matrix ");

    table_builder tb;
    tb.add_column("Category",    {justify::left,   16, 22});
    tb.add_column("Feature",     {justify::left,   28, 36});
    tb.add_column("Status",      {justify::center,  8, 12});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});
    tb.shadow();

    auto ok = "[green]Ready[/]";

    tb.add_row({"File I/O",       "Create CFB v3/v4 files",          ok});
    tb.add_row({"File I/O",       "Open/read existing CFB files",    ok});
    tb.add_row({"File I/O",       "Flush/persist to disk",           ok});
    tb.add_row({"Storage",        "Create/open sub-storages",        ok});
    tb.add_row({"Storage",        "Nested hierarchy (unlimited)",    ok});
    tb.add_row({"Storage",        "Enumerate children",              ok});
    tb.add_row({"Storage",        "Remove entries",                  ok});
    tb.add_row({"Storage",        "Rename entries",                  ok});
    tb.add_row({"Storage",        "Exists check",                    ok});
    tb.add_row({"Stream",         "Create/open streams",             ok});
    tb.add_row({"Stream",         "Read/write at offset",            ok});
    tb.add_row({"Stream",         "Resize streams",                  ok});
    tb.add_row({"Stream",         "Mini stream (<4096 bytes)",       ok});
    tb.add_row({"Stream",         "Regular stream (>=4096 bytes)",   ok});
    tb.add_row({"Stream",         "Copy stream data",                ok});
    tb.add_row({"Metadata",       "CLSID get/set",                  ok});
    tb.add_row({"Metadata",       "State bits get/set",              ok});
    tb.add_row({"Metadata",       "Created/modified timestamps",    ok});
    tb.add_row({"Property Set",   "OLE property set read/write",    ok});
    tb.add_row({"Property Set",   "Summary Information (FMTID)",    ok});
    tb.add_row({"Property Set",   "String/I4/Bool properties",      ok});
    tb.add_row({"Interop",        "Win32 IStorage compatible",      ok});
    tb.add_row({"Interop",        "Win32 IStream compatible",       ok});
    tb.add_row({"Interop",        "STATSTG conformance",            ok});

    con.print_widget(tb, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  CFB Format Comparison (v3 vs v4)
// ═══════════════════════════════════════════════════════════════════════

static void show_version_comparison(console& con) {
    section(con, " CFB Version Comparison ");

    table_builder tb;
    tb.add_column("Property",          {justify::left,   24, 30});
    tb.add_column("CFB v3",            {justify::center, 14, 18});
    tb.add_column("CFB v4",            {justify::center, 14, 18});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_yellow, {}, attr::bold});

    tb.add_row({"Sector size",         "512 bytes",       "4096 bytes"});
    tb.add_row({"Max file size",       "~2 GB",           "~16 TB"});
    tb.add_row({"Mini stream cutoff",  "4096 bytes",      "4096 bytes"});
    tb.add_row({"Mini sector size",    "64 bytes",        "64 bytes"});
    tb.add_row({"Directory entries",   "128 bytes each",  "128 bytes each"});
    tb.add_row({"FAT sector entries",  "128 per sector",  "1024 per sector"});
    tb.add_row({"DIFAT entries",       "109 in header",   "109 in header"});
    tb.add_row({"Compatibility",       "[green]Universal[/]", "[yellow]Office 2007+[/]"});

    con.print_widget(tb, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Performance Metrics (simulated)
// ═══════════════════════════════════════════════════════════════════════

static void show_performance(console& con) {
    section(con, " Performance Metrics ");

    con.print("[dim]Simulated benchmarks based on typical CFB operations:[/]");
    con.newline();

    // Simulated benchmark data (us per operation)
    struct perf_entry {
        const char* name;
        double v3_us;
        double v4_us;
    };

    perf_entry entries[] = {
        {"Create empty file",       45.0,   52.0},
        {"Open existing file",      38.0,   41.0},
        {"Create stream",           12.0,   11.0},
        {"Write 1KB mini stream",   18.0,   16.0},
        {"Write 8KB regular",       35.0,   28.0},
        {"Read 1KB mini stream",    14.0,   12.0},
        {"Read 8KB regular",        25.0,   20.0},
        {"Flush to disk",           85.0,   92.0},
        {"Enumerate 20 children",   22.0,   20.0},
        {"Property set serialize",  55.0,   55.0},
    };

    table_builder tb;
    tb.add_column("Operation",       {justify::left,   24, 30});
    tb.add_column("v3 (us)",         {justify::right,  10, 14});
    tb.add_column("v4 (us)",         {justify::right,  10, 14});
    tb.add_column("Diff",            {justify::right,  10, 14});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_yellow, {}, attr::bold});

    for (auto& e : entries) {
        char v3[16], v4[16], diff[32];
        std::snprintf(v3, sizeof(v3), "%.1f", e.v3_us);
        std::snprintf(v4, sizeof(v4), "%.1f", e.v4_us);
        double pct = (e.v4_us - e.v3_us) / e.v3_us * 100.0;
        if (pct <= 0)
            std::snprintf(diff, sizeof(diff), "[green]%.0f%%[/]", pct);
        else
            std::snprintf(diff, sizeof(diff), "[yellow]+%.0f%%[/]", pct);
        tb.add_row({e.name, v3, v4, diff});
    }
    con.print_widget(tb, W);

    // Bar chart: v3 vs v4 for key operations
    con.newline();
    con.print("[dim]Read throughput comparison (higher = better):[/]");

    std::vector<float> throughput = {
        1000.0f / 18.0f,   // v3 write 1KB
        1000.0f / 16.0f,   // v4 write 1KB
        8000.0f / 35.0f,   // v3 write 8KB
        8000.0f / 28.0f,   // v4 write 8KB
        1000.0f / 14.0f,   // v3 read 1KB
        1000.0f / 12.0f,   // v4 read 1KB
        8000.0f / 25.0f,   // v3 read 8KB
        8000.0f / 20.0f,   // v4 read 8KB
    };
    con.print_widget(
        bar_chart_builder(throughput, 6)
            .labels({"W1K3","W1K4","W8K3","W8K4","R1K3","R1K4","R8K3","R8K4"})
            .style_override(style{colors::bright_green}),
        W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Architecture Overview
// ═══════════════════════════════════════════════════════════════════════

static void show_architecture(console& con) {
    section(con, " Architecture Overview ");

    columns_builder cols;

    // Left: layer diagram
    {
        rows_builder layers;
        layers.add(text_builder("[bold bright_white on_blue]    compound_file API    [/]"));
        layers.add(text_builder("[dim]          |[/]"));
        layers.add(text_builder("[bold bright_white on_magenta]   storage / stream     [/]"));
        layers.add(text_builder("[dim]          |[/]"));
        layers.add(text_builder("[bold bright_white on_cyan]   FAT / mini-FAT       [/]"));
        layers.add(text_builder("[dim]          |[/]"));
        layers.add(text_builder("[bold bright_white on_green]   sector I/O layer     [/]"));
        layers.add(text_builder("[dim]          |[/]"));
        layers.add(text_builder("[bold bright_white on_red]   file (disk / memory) [/]"));
        layers.gap(0);

        panel_builder lp(std::move(layers));
        lp.title("Layers");
        lp.border(border_style::rounded);
        cols.add(std::move(lp), 1);
    }

    // Right: key types
    {
        rows_builder types;
        types.add(text_builder("[bold]compound_file[/]  — top-level file handle"));
        types.add(text_builder("[bold]storage[/]        — directory-like container"));
        types.add(text_builder("[bold]stream[/]         — byte-stream data"));
        types.add(text_builder("[bold]property_set[/]   — OLE metadata"));
        types.add(text_builder("[bold]guid[/]           — 128-bit CLSID"));
        types.add(text_builder("[bold]cfb_version[/]    — v3 or v4 selector"));
        types.add(text_builder("[bold]open_mode[/]      — read / read_write"));
        types.add(text_builder("[bold]error[/]          — std::expected error"));
        types.gap(0);

        panel_builder rp(std::move(types));
        rp.title("Key Types");
        rp.border(border_style::rounded);
        cols.add(std::move(rp), 1);
    }

    cols.gap(1);
    con.print_widget(std::move(cols), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Test Coverage
// ═══════════════════════════════════════════════════════════════════════

static void show_test_coverage(console& con) {
    section(con, " Test Coverage ");

    columns_builder cols;

    // Gauge panel
    {
        rows_builder gauges;
        gauges.add(text_builder("[bold]Win32 Conformance[/]  924/924"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Unit Tests[/]         229/229"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Total (ctest)[/]      1153/1153"));
        gauges.add(make_gauge(1.0f));
        gauges.gap(0);

        panel_builder gp(std::move(gauges));
        gp.title("Pass Rate");
        gp.border(border_style::rounded);
        cols.add(std::move(gp), 1);
    }

    // Test file breakdown
    {
        table_builder tb;
        tb.add_column("Test Category",  {justify::left,   20, 26});
        tb.add_column("Count",          {justify::right,   6, 10});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});

        tb.add_row({"Basic conformance",     "97"});
        tb.add_row({"Stress: read/write",    "56"});
        tb.add_row({"Stress: hierarchy",     "48"});
        tb.add_row({"Stress: property set",  "42"});
        tb.add_row({"Stress: cross-API",     "38"});
        tb.add_row({"Stress: data patterns", "44"});
        tb.add_row({"Stress: roundtrip",     "36"});
        tb.add_row({"Stress: exists/stat",   "52"});
        tb.add_row({"Other stress tests",    "511"});
        tb.add_row({"[bold]Total[/]",        "[bold]924[/]"});

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
    show_features(con);
    show_version_comparison(con);
    show_performance(con);
    show_architecture(con);
    show_test_coverage(con);

    con.newline();
    con.print_widget(
        status_bar_builder()
            .left("[bold] stout [/]")
            .center("Compound File Binary Library")
            .right("24 features | 1153 tests")
            .style_override(style{colors::bright_white, color::from_rgb(30, 50, 30)}),
        W);
    con.newline();
    con.print_widget(
        rule_builder(" stout Dashboard Complete ")
            .rule_style(style{colors::bright_green, {}, attr::bold})
            .character(U'\x2550'), W);
    con.newline();

    return 0;
}
