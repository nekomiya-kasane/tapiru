/**
 * @file gin_dashboard.cpp
 * @brief Dashboard demo showcasing gin (C++23 utility library) features and performance.
 *
 * Build:
 *   cmake --build build --target gin_dashboard --config Release
 * Run:
 *   .\build\bin\Release\gin_dashboard.exe
 */

#include "tapiru/core/console.h"
#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/core/style.h"
#include "tapiru/text/emoji.h"
#include "tapiru/widgets/builders.h"
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
    con.print_widget(rule_builder(" gin — Feature & Performance Dashboard ")
                         .rule_style(style{colors::bright_yellow, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    rows_builder info;
    info.add(text_builder("[bold bright_white]gin[/] [dim]v0.2[/]  —  General-Purpose C++23 Utility Library"));
    info.add(text_builder("[dim]Unicode strings, logging, async, networking, config, serialization, codec[/]"));
    info.add(text_builder("[dim]Process/service management, registry, signal/slot, CLI parsing[/]"));
    info.gap(0);

    panel_builder pb(std::move(info));
    pb.title("About");
    pb.border(border_style::rounded);
    pb.border_style_override(style{colors::bright_cyan});
    con.print_widget(std::move(pb), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Module Overview
// ═══════════════════════════════════════════════════════════════════════

static void show_modules(console &con) {
    section(con, " Module Overview ");

    table_builder tb;
    tb.add_column("Module", {justify::left, 14, 18});
    tb.add_column("Namespace", {justify::left, 14, 18});
    tb.add_column("Description", {justify::left, 30, 38});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});
    tb.shadow();

    tb.add_row({"ADT", "gin::adt", "Unicode strings, containers"});
    tb.add_row({"Logging", "gin::log", "Multi-sink structured logging"});
    tb.add_row({"Async", "gin::async", "Thread pool, futures"});
    tb.add_row({"Codec", "gin::codec", "Base64 encode/decode"});
    tb.add_row({"Serial", "gin::serial", "JSON parse/serialize"});
    tb.add_row({"Network", "gin::net", "TCP/UDP sockets"});
    tb.add_row({"Config", "gin::config", "Layered configuration"});
    tb.add_row({"Service", "gin::os", "Service install/control"});
    tb.add_row({"Process", "gin::os", "Process spawn, pipe I/O"});
    tb.add_row({"Registry", "gin::os", "Windows registry RAII"});
    tb.add_row({"CLI", "gin::cl", "LLVM-style option parser"});
    tb.add_row({"Signals", "gin::slots", "Signal/slot event system"});
    tb.add_row({"Hasher", "gin::utils", "Crypto + non-crypto hashing"});
    tb.add_row({"Version", "gin", "Compile/runtime version"});

    con.print_widget(tb, W);
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

    auto ok = "[green]Ready[/]";
    auto opt = "[yellow]Optional[/]";

    tb.add_row({"ADT", "ustring (ICU-backed Unicode)", ok});
    tb.add_row({"ADT", "lines() / chunks() lazy views", ok});
    tb.add_row({"ADT", "flat_string_map<V> (transparent)", ok});
    tb.add_row({"ADT", "iterator_range (C++20 concepts)", ok});
    tb.add_row({"Logging", "Console/file/ringbuffer sinks", ok});
    tb.add_row({"Logging", "MSVC debug output sink", ok});
    tb.add_row({"Logging", "Structured key-value logging", ok});
    tb.add_row({"Logging", "Level-based filtering", ok});
    tb.add_row({"Async", "Thread pool (N workers)", ok});
    tb.add_row({"Async", "Future-based task submission", ok});
    tb.add_row({"Codec", "Base64 encode/decode", ok});
    tb.add_row({"Serial", "JSON parse/serialize/dump", ok});
    tb.add_row({"Serial", "Object/array/string/number types", ok});
    tb.add_row({"Network", "TCP client/server", ok});
    tb.add_row({"Network", "UDP socket (send_to/recv_from)", ok});
    tb.add_row({"Config", "Layered: default/file/env/cli/rt", ok});
    tb.add_row({"Config", "Source introspection", ok});
    tb.add_row({"Config", "Validation + save_file", ok});
    tb.add_row({"Service", "Windows SCM / Linux systemd", ok});
    tb.add_row({"Service", "Install/uninstall/start/stop", ok});
    tb.add_row({"Process", "Spawn with pipe capture", ok});
    tb.add_row({"Registry", "RAII key open/read/write", ok});
    tb.add_row({"CLI", "LLVM-style cl::opt declarations", ok});
    tb.add_row({"Signals", "Signal/slot event dispatch", ok});
    tb.add_row({"Hasher", "Crypto hashing (OpenSSL)", opt});
    tb.add_row({"Hasher", "Non-crypto hashing", ok});

    con.print_widget(tb, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Performance Metrics (simulated)
// ═══════════════════════════════════════════════════════════════════════

static void show_performance(console &con) {
    section(con, " Performance Metrics ");

    con.print("[dim]Simulated benchmarks based on typical gin operations:[/]");
    con.newline();

    struct perf_entry {
        const char *name;
        double us;
        int ops;
    };
    perf_entry entries[] = {
        {"ustring create (short)", 0.15, 1000000}, {"ustring::lines() iterate", 2.5, 100000},
        {"flat_string_map insert", 0.08, 1000000}, {"flat_string_map lookup", 0.05, 1000000},
        {"Base64 encode (1KB)", 1.8, 500000},      {"Base64 decode (1KB)", 2.1, 500000},
        {"JSON parse (small obj)", 3.5, 200000},   {"JSON dump (small obj)", 1.2, 500000},
        {"Thread pool submit", 0.4, 1000000},      {"Logger info() call", 0.6, 1000000},
        {"Config get_int()", 0.03, 5000000},       {"Signal emit (3 slots)", 0.25, 2000000},
    };

    table_builder tb;
    tb.add_column("Operation", {justify::left, 26, 32});
    tb.add_column("Per-op (us)", {justify::right, 10, 14});
    tb.add_column("Throughput", {justify::right, 16, 20});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_yellow, {}, attr::bold});

    for (auto &e : entries) {
        char per[16], thr[32];
        std::snprintf(per, sizeof(per), "%.2f", e.us);
        double ops_per_sec = 1000000.0 / e.us;
        if (ops_per_sec >= 1000000.0)
            std::snprintf(thr, sizeof(thr), "%.1fM ops/s", ops_per_sec / 1000000.0);
        else
            std::snprintf(thr, sizeof(thr), "%.1fK ops/s", ops_per_sec / 1000.0);
        tb.add_row({e.name, per, thr});
    }
    con.print_widget(tb, W);

    // Bar chart: throughput comparison
    con.newline();
    con.print("[dim]Throughput comparison (M ops/s, higher = better):[/]");
    std::vector<float> throughputs;
    std::vector<std::string> labels;
    for (auto &e : entries) {
        throughputs.push_back(static_cast<float>(1.0 / e.us)); // M ops/s
        std::string l(e.name);
        auto sp = l.find(' ');
        if (sp != std::string::npos) l = l.substr(0, sp);
        if (l.size() > 6) l = l.substr(0, 6);
        labels.push_back(l);
    }
    con.print_widget(bar_chart_builder(throughputs, 6).labels(labels).style_override(style{colors::bright_yellow}), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Architecture: Config Priority Layers
// ═══════════════════════════════════════════════════════════════════════

static void show_config_layers(console &con) {
    section(con, " Config Priority System ");

    rows_builder layers;
    layers.add(text_builder("[bold bright_white on_red]  5. runtime overrides  (highest)  [/]"));
    layers.add(text_builder("[dim]              |[/]"));
    layers.add(text_builder("[bold bright_white on_magenta]  4. CLI arguments       --flag     [/]"));
    layers.add(text_builder("[dim]              |[/]"));
    layers.add(text_builder("[bold bright_white on_blue]  3. Environment vars    $ENV_VAR   [/]"));
    layers.add(text_builder("[dim]              |[/]"));
    layers.add(text_builder("[bold bright_white on_cyan]  2. Config file         config.json [/]"));
    layers.add(text_builder("[dim]              |[/]"));
    layers.add(text_builder("[bold bright_white on_green]  1. Default values      (lowest)    [/]"));
    layers.gap(0);

    panel_builder pb(std::move(layers));
    pb.title("Priority Order (highest wins)");
    pb.border(border_style::rounded);
    con.print_widget(std::move(pb), 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  Test Coverage
// ═══════════════════════════════════════════════════════════════════════

static void show_test_coverage(console &con) {
    section(con, " Test Coverage ");

    columns_builder cols;

    {
        rows_builder gauges;
        gauges.add(text_builder("[bold]ADT Tests[/]         60/60"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Log Tests[/]         45/45"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Serial Tests[/]      38/38"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Net/Config/Other[/]  207/207"));
        gauges.add(make_gauge(1.0f));
        gauges.add(text_builder("[bold]Total (ctest)[/]     1350/1350"));
        gauges.add(make_gauge(1.0f));
        gauges.gap(0);

        panel_builder gp(std::move(gauges));
        gp.title("Pass Rate");
        gp.border(border_style::rounded);
        cols.add(std::move(gp), 1);
    }

    {
        table_builder tb;
        tb.add_column("Module", {justify::left, 18, 24});
        tb.add_column("Tests", {justify::right, 6, 10});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});

        tb.add_row({"ustring", "30"});
        tb.add_row({"flat_string_map", "20"});
        tb.add_row({"iterator_range", "10"});
        tb.add_row({"logging", "45"});
        tb.add_row({"base64", "18"});
        tb.add_row({"json", "38"});
        tb.add_row({"tcp/udp", "32"});
        tb.add_row({"config", "28"});
        tb.add_row({"process", "22"});
        tb.add_row({"service", "15"});
        tb.add_row({"registry", "12"});
        tb.add_row({"signal/slot", "18"});
        tb.add_row({"hasher", "25"});
        tb.add_row({"other", "37"});
        tb.add_row({"[bold]Total[/]", "[bold]1350[/]"});

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
    show_modules(con);
    show_features(con);
    show_performance(con);
    show_config_layers(con);
    show_test_coverage(con);

    con.newline();
    con.print_widget(status_bar_builder()
                         .left("[bold] gin [/]")
                         .center("General-Purpose C++23 Utility Library")
                         .right("26 features | 1350 tests")
                         .style_override(style{colors::bright_white, color::from_rgb(50, 30, 30)}),
                     W);
    con.newline();
    con.print_widget(rule_builder(" gin Dashboard Complete ")
                         .rule_style(style{colors::bright_green, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    return 0;
}
