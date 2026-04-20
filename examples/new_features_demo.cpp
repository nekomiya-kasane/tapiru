/**
 * @file new_features_demo.cpp
 * @brief Comprehensive demo of all newly added tapiru features.
 *
 * Covers: rows_builder, spacer_builder, status_bar_builder, menu_builder,
 *         popup_builder, table shadow/glow/gradient, container opacity,
 *         widget keys, z_order, and interactive widget key support.
 *
 * Build:
 *   cmake --build build --target tapiru_new_features_demo --config Debug
 * Run:
 *   .\build\bin\Debug\tapiru_new_features_demo.exe
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
#include "tapiru/core/style.h"

// ── Text ────────────────────────────────────────────────────────────────
#include "tapiru/text/markup.h"

// ── Widgets ─────────────────────────────────────────────────────────────
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/interactive.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/popup.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/spinner.h"
#include "tapiru/widgets/status_bar.h"

using namespace tapiru;

static constexpr uint32_t W = 76;

// ═══════════════════════════════════════════════════════════════════════
//  Helper
// ═══════════════════════════════════════════════════════════════════════

static void section(console& con, const char* title) {
    con.newline();
    con.print_widget(rule_builder(title).rule_style(style{colors::bright_cyan, {}, attr::bold}), W);
    con.newline();
}

static void subsection(console& con, const char* desc) {
    con.print(std::string("[dim]") + desc + "[/]");
}

// ═══════════════════════════════════════════════════════════════════════
//  1. Rows Layout — vertical stacking with flex & gap
// ═══════════════════════════════════════════════════════════════════════

static void demo_rows_basic(console& con) {
    section(con, " 1. Rows Layout ");

    // 1a. Simple vertical stack
    subsection(con, "1a. Basic vertical stack (3 text rows, gap=0):");
    {
        rows_builder rb;
        rb.add(text_builder("[bold cyan]Header row[/]"));
        rb.add(text_builder("  Content line 1"));
        rb.add(text_builder("  Content line 2"));
        rb.add(text_builder("[dim]  Footer row[/]"));
        rb.gap(0);
        con.print_widget(std::move(rb), W);
    }

    con.newline();

    // 1b. Rows with gap
    subsection(con, "1b. Rows with gap=1:");
    {
        rows_builder rb;
        rb.add(text_builder("[bold green]Section A[/]"));
        rb.add(text_builder("[bold yellow]Section B[/]"));
        rb.add(text_builder("[bold red]Section C[/]"));
        rb.gap(1);
        con.print_widget(std::move(rb), W);
    }

    con.newline();

    // 1c. Nested: rows inside a panel
    subsection(con, "1c. Rows inside a panel:");
    {
        rows_builder inner;
        inner.add(text_builder("[bold]Name:[/]  tapiru"));
        inner.add(text_builder("[bold]Lang:[/]  C++23"));
        inner.add(text_builder("[bold]Type:[/]  Terminal UI Library"));
        inner.gap(0);

        panel_builder pb(std::move(inner));
        pb.title("Project Info");
        pb.border(border_style::rounded);
        con.print_widget(std::move(pb), 40);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  2. Spacer — flexible empty space for centering
// ═══════════════════════════════════════════════════════════════════════

static void demo_spacer(console& con) {
    section(con, " 2. Spacer Widget ");

    // 2a. Spacer between columns to push content apart
    subsection(con, "2a. Spacer pushing columns apart (left | spacer | right):");
    {
        columns_builder cols;
        cols.add(text_builder("[bold green]LEFT[/]"));
        cols.add(spacer_builder(), 1);  // flex=1 fills remaining space
        cols.add(text_builder("[bold red]RIGHT[/]"));
        cols.gap(0);
        con.print_widget(std::move(cols), 50);
    }

    con.newline();

    // 2b. Centering with two spacers
    subsection(con, "2b. Centering with spacer | content | spacer:");
    {
        columns_builder cols;
        cols.add(spacer_builder(), 1);
        cols.add(text_builder("[bold bright_yellow]CENTERED[/]"));
        cols.add(spacer_builder(), 1);
        cols.gap(0);
        con.print_widget(std::move(cols), 50);
    }

    con.newline();

    // 2c. Three-section layout (status-bar style using raw columns)
    subsection(con, "2c. Three-section layout: [left] [spacer] [center] [spacer] [right]:");
    {
        columns_builder cols;
        cols.add(text_builder("[bold] INSERT [/]"));
        cols.add(spacer_builder(), 1);
        cols.add(text_builder("[bold cyan]main.cpp[/]"));
        cols.add(spacer_builder(), 1);
        cols.add(text_builder("[dim]UTF-8 | LF | C++[/]"));
        cols.gap(0);

        panel_builder pb(std::move(cols));
        pb.border(border_style::ascii);
        pb.border_style_override(style{colors::bright_black});
        con.print_widget(std::move(pb), 60);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  3. Status Bar — composite left/center/right widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_status_bar(console& con) {
    section(con, " 3. Status Bar ");

    // 3a. Editor-style status bar
    subsection(con, "3a. Editor status bar (dark blue background):");
    con.print_widget(
        status_bar_builder()
            .left("[bold bright_white] NORMAL [/]")
            .center("[bold]main.cpp[/] [dim](modified)[/]")
            .right("[dim]Ln 142, Col 28[/]")
            .style_override(style{colors::bright_white, color::from_rgb(30, 30, 80)}),
        W);

    con.newline();

    // 3b. Build status bar
    subsection(con, "3b. Build status bar (green background):");
    con.print_widget(
        status_bar_builder()
            .left("[bold] BUILD [/]")
            .center("[bold]Release x64[/]")
            .right("[bold green]0 errors, 2 warnings[/]")
            .style_override(style{colors::bright_white, color::from_rgb(20, 60, 20)}),
        W);

    con.newline();

    // 3c. Error status bar
    subsection(con, "3c. Error status bar (red background):");
    con.print_widget(
        status_bar_builder()
            .left("[bold] ERROR [/]")
            .center("Connection refused: ECONNREFUSED 127.0.0.1:5432")
            .right("[bold]Retry?[/]")
            .style_override(style{colors::bright_white, color::from_rgb(120, 20, 20)}),
        W);

    con.newline();

    // 3d. Minimal status bar (no style)
    subsection(con, "3d. Minimal status bar (default style):");
    con.print_widget(
        status_bar_builder()
            .left("tapiru v0.1.0")
            .right("Ready"),
        W);
}

// ═══════════════════════════════════════════════════════════════════════
//  4. Menu Widget — dropdown/context menu
// ═══════════════════════════════════════════════════════════════════════

static void demo_menu(console& con) {
    section(con, " 4. Menu Widget ");

    // 4a. File menu with cursor on item 0
    subsection(con, "4a. File menu (cursor on 'New File', with shadow):");
    {
        int cursor = 0;
        con.print_widget(
            menu_builder()
                .add_item("New File",       "Ctrl+N")
                .add_item("Open File...",   "Ctrl+O")
                .add_item("Open Recent",    "")
                .add_separator()
                .add_item("Save",           "Ctrl+S")
                .add_item("Save As...",     "Ctrl+Shift+S")
                .add_separator()
                .add_item("Exit",           "Ctrl+Q")
                .cursor(&cursor)
                .border(border_style::rounded)
                .highlight_style(style{colors::bright_white, colors::blue, attr::bold})
                .shortcut_style(style{colors::bright_black})
                .item_style(style{colors::white})
                .shadow(),
            35);
    }

    con.newline();

    // 4b. Edit menu with cursor on item 2
    subsection(con, "4b. Edit menu (cursor on 'Paste', heavy border, glow):");
    {
        int cursor = 2;
        con.print_widget(
            menu_builder()
                .add_item("Undo",   "Ctrl+Z")
                .add_item("Redo",   "Ctrl+Y")
                .add_item("Paste",  "Ctrl+V")
                .add_item("Copy",   "Ctrl+C")
                .add_item("Cut",    "Ctrl+X")
                .add_separator()
                .add_item("Select All", "Ctrl+A")
                .cursor(&cursor)
                .border(border_style::heavy)
                .border_style_override(style{colors::cyan})
                .highlight_style(style{colors::black, colors::bright_cyan, attr::bold})
                .shortcut_style(style{colors::bright_black})
                .glow(color::from_rgb(0, 180, 200)),
            30);
    }

    con.newline();

    // 4c. Context menu with no cursor (no selection)
    subsection(con, "4c. Context menu (no cursor, single border):");
    {
        con.print_widget(
            menu_builder()
                .add_item("Inspect Element")
                .add_item("View Source")
                .add_separator()
                .add_item("Reload",    "F5")
                .add_item("Hard Reload", "Ctrl+Shift+R")
                .border(border_style::ascii),
            28);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  5. Popup Widget — modal overlay with dimmed background
// ═══════════════════════════════════════════════════════════════════════

static void demo_popup(console& con) {
    section(con, " 5. Popup Widget ");

    // 5a. Centered popup with title and shadow
    subsection(con, "5a. Centered popup with title and shadow:");
    {
        auto content = text_builder(
            "[bold]Are you sure you want to exit?[/]\n"
            "\n"
            "All unsaved changes will be lost.\n"
            "\n"
            "[dim]Press [bold]Y[/bold] to confirm, [bold]N[/bold] to cancel.[/]"
        );
        popup_builder pb(std::move(content));
        pb.title("Confirm Exit");
        pb.anchor(popup_anchor::center);
        pb.border(border_style::rounded);
        pb.border_style_override(style{colors::bright_yellow});
        pb.shadow();
        con.print_widget(std::move(pb), 50);
    }

    con.newline();

    // 5b. Popup with glow effect
    subsection(con, "5b. Popup with cyan glow:");
    {
        auto content = text_builder(
            "[bold cyan]Notification[/]\n"
            "\n"
            "Build completed successfully!\n"
            "[green]0 errors[/], [yellow]2 warnings[/]"
        );
        popup_builder pb(std::move(content));
        pb.title("Build Result");
        pb.anchor(popup_anchor::center);
        pb.border(border_style::heavy);
        pb.border_style_override(style{colors::bright_cyan});
        pb.glow(color::from_rgb(0, 200, 255));
        con.print_widget(std::move(pb), 45);
    }

    con.newline();

    // 5c. Popup containing a table
    subsection(con, "5c. Popup wrapping a table widget:");
    {
        table_builder tb;
        tb.add_column("Key", {justify::left, 10, 15});
        tb.add_column("Value", {justify::left, 15, 25});
        tb.border(border_style::ascii);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});
        tb.add_row({"Version",  "0.1.0"});
        tb.add_row({"Compiler", "MSVC 19.41"});
        tb.add_row({"Standard", "C++23"});
        tb.add_row({"Platform", "Windows x64"});

        popup_builder pb(std::move(tb));
        pb.title("System Info");
        pb.border(border_style::rounded);
        pb.shadow();
        con.print_widget(std::move(pb), 50);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  6. Table Visual Features — shadow, glow, gradient
// ═══════════════════════════════════════════════════════════════════════

static void demo_table_visuals(console& con) {
    section(con, " 6. Table Visual Features ");

    // 6a. Table with shadow
    subsection(con, "6a. Table with drop shadow:");
    {
        table_builder tb;
        tb.add_column("Feature", {justify::left, 18, 25});
        tb.add_column("Status", {justify::center, 10, 15});
        tb.add_column("Priority", {justify::right, 8, 12});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});
        tb.shadow();

        tb.add_row({"Rows layout",     "[green]Done[/]",    "[bold]High[/]"});
        tb.add_row({"Spacer widget",   "[green]Done[/]",    "[bold]High[/]"});
        tb.add_row({"Status bar",      "[green]Done[/]",    "[bold]High[/]"});
        tb.add_row({"Menu widget",     "[green]Done[/]",    "[bold]High[/]"});
        tb.add_row({"Popup widget",    "[green]Done[/]",    "[bold]High[/]"});
        tb.add_row({"Table shadow",    "[green]Done[/]",    "Medium"});
        tb.add_row({"Container opacity","[green]Done[/]",   "Medium"});
        tb.add_row({"Widget keys",     "[green]Done[/]",    "Medium"});

        con.print_widget(tb, 60);
    }

    con.newline();

    // 6b. Table with glow
    subsection(con, "6b. Table with green glow:");
    {
        table_builder tb;
        tb.add_column("Test", {justify::left, 20, 30});
        tb.add_column("Result", {justify::center, 8, 12});
        tb.border(border_style::heavy);
        tb.border_style_override(style{colors::green});
        tb.header_style(style{colors::bright_green, {}, attr::bold});
        tb.glow(color::from_rgb(0, 200, 0));

        tb.add_row({"Unit tests",       "[green]PASS[/]"});
        tb.add_row({"Integration tests","[green]PASS[/]"});
        tb.add_row({"Regression tests", "[green]PASS[/]"});
        tb.add_row({"Performance tests","[yellow]WARN[/]"});

        con.print_widget(tb, 50);
    }

    con.newline();

    // 6c. Table with custom shadow parameters
    subsection(con, "6c. Table with custom shadow (offset=3,2, red tint):");
    {
        table_builder tb;
        tb.add_column("Error Code", {justify::left, 12, 15});
        tb.add_column("Message", {justify::left, 20, 30});
        tb.border(border_style::rounded);
        tb.border_style_override(style{colors::red});
        tb.header_style(style{colors::bright_red, {}, attr::bold});
        tb.shadow(3, 2, 2, color::from_rgb(180, 0, 0), 100);

        tb.add_row({"E001", "Null pointer dereference"});
        tb.add_row({"E002", "Buffer overflow detected"});
        tb.add_row({"E003", "Stack corruption"});

        con.print_widget(tb, 55);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  7. Widget Keys — stable identity for all builders
// ═══════════════════════════════════════════════════════════════════════

static void demo_widget_keys(console& con) {
    section(con, " 7. Widget Keys ");

    subsection(con, "All builders now support .key() for stable identity:");
    con.newline();

    // Show keyed widgets in a rows layout
    {
        rows_builder rb;
        rb.add(
            text_builder("[bold]text_builder[/] with .key(\"header\")")
                .key("header")
        );
        rb.add(
            text_builder("[bold]panel_builder[/] with .key(\"info-panel\")")
                .key("info-panel-text")
        );
        rb.gap(0);

        panel_builder pb(std::move(rb));
        pb.title("Keyed Widgets");
        pb.border(border_style::rounded);
        pb.key("info-panel");
        con.print_widget(std::move(pb), 50);
    }

    con.newline();

    subsection(con, "Keyed interactive widgets:");
    {
        table_builder tb;
        tb.add_column("Widget", {justify::left, 20, 30});
        tb.add_column("Key Example", {justify::left, 25, 35});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});
        tb.key("key-demo-table");

        tb.add_row({"button_builder",         ".key(\"submit-btn\")"});
        tb.add_row({"checkbox_builder",       ".key(\"agree-cb\")"});
        tb.add_row({"radio_group_builder",    ".key(\"theme-radio\")"});
        tb.add_row({"selectable_list_builder",".key(\"file-list\")"});
        tb.add_row({"text_input_builder",     ".key(\"search-input\")"});
        tb.add_row({"slider_builder",         ".key(\"volume-slider\")"});
        tb.add_row({"line_chart_builder",     ".key(\"cpu-chart\")"});
        tb.add_row({"bar_chart_builder",      ".key(\"mem-chart\")"});
        tb.add_row({"scatter_builder",        ".key(\"perf-scatter\")"});
        tb.add_row({"image_builder",          ".key(\"logo-img\")"});
        tb.add_row({"progress_builder",       ".key(\"dl-progress\")"});
        tb.add_row({"spinner_builder",        ".key(\"load-spinner\")"});
        tb.add_row({"markdown_builder",       ".key(\"readme-md\")"});

        con.print_widget(tb, 65);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  8. Complex Layouts — nesting rows, columns, panels, spacers
// ═══════════════════════════════════════════════════════════════════════

static void demo_complex_layout(console& con) {
    section(con, " 8. Complex Nested Layouts ");

    // 8a. Dashboard-style layout: header + two-column body + footer
    subsection(con, "8a. Dashboard: header + 2-column body + status bar:");
    {
        rows_builder dashboard;

        // Header
        dashboard.add(
            text_builder("[bold bright_white on_blue]  Dashboard — tapiru v0.1.0  [/]")
                .key("dash-header")
        );

        // Body: two columns
        {
            columns_builder body;

            // Left column: progress tasks
            {
                auto t1 = std::make_shared<progress_task>("CPU Usage", 100);
                auto t2 = std::make_shared<progress_task>("Memory",    100);
                auto t3 = std::make_shared<progress_task>("Disk I/O",  100);
                t1->advance(73);
                t2->advance(45);
                t3->advance(12);

                panel_builder left_panel(
                    progress_builder()
                        .add_task(t1)
                        .add_task(t2)
                        .add_task(t3)
                        .bar_width(18)
                        .key("sys-progress")
                );
                left_panel.title("System");
                left_panel.border(border_style::rounded);
                left_panel.key("sys-panel");
                body.add(std::move(left_panel), 1);
            }

            // Right column: mini chart
            {
                std::vector<float> data;
                for (int i = 0; i < 40; ++i)
                    data.push_back(static_cast<float>(std::sin(i * 0.2) * 30 + 50));

                panel_builder right_panel(
                    line_chart_builder(data, 30, 5)
                        .style_override(style{colors::bright_green})
                        .key("cpu-chart")
                );
                right_panel.title("CPU History");
                right_panel.border(border_style::rounded);
                right_panel.key("chart-panel");
                body.add(std::move(right_panel), 1);
            }

            body.gap(1);
            body.key("dash-body");
            dashboard.add(std::move(body));
        }

        // Footer: status bar
        dashboard.add(
            status_bar_builder()
                .left("[bold] ONLINE [/]")
                .center("[dim]Uptime: 3d 14h 22m[/]")
                .right("[green]All systems nominal[/]")
                .style_override(style{colors::bright_white, color::from_rgb(30, 30, 60)})
        );

        dashboard.gap(0);
        dashboard.key("dashboard");
        con.print_widget(std::move(dashboard), W);
    }

    con.newline();

    // 8b. Sidebar + content layout
    subsection(con, "8b. Sidebar + content area:");
    {
        columns_builder layout;

        // Sidebar: menu
        {
            int cursor = 2;
            layout.add(
                menu_builder()
                    .add_item("Home")
                    .add_item("Projects")
                    .add_item("Settings")
                    .add_separator()
                    .add_item("Help")
                    .add_item("About")
                    .cursor(&cursor)
                    .highlight_style(style{colors::bright_white, colors::blue, attr::bold})
                    .border(border_style::rounded)
                    .key("sidebar-menu")
            );
        }

        // Content area
        {
            rows_builder content;
            content.add(text_builder("[bold bright_cyan]Settings[/]"));
            content.add(rule_builder().character(U'\x2500'));

            table_builder tb;
            tb.add_column("Option", {justify::left, 15, 20});
            tb.add_column("Value", {justify::left, 15, 25});
            tb.border(border_style::none);
            tb.show_header(false);
            tb.add_row({"Theme",     "[cyan]Dark[/]"});
            tb.add_row({"Font Size", "[cyan]14px[/]"});
            tb.add_row({"Tab Width", "[cyan]4[/]"});
            tb.add_row({"Word Wrap", "[green]On[/]"});
            tb.add_row({"Auto Save", "[red]Off[/]"});
            tb.key("settings-table");
            content.add(std::move(tb));

            content.gap(0);
            content.key("settings-content");

            panel_builder content_panel(std::move(content));
            content_panel.title("Content");
            content_panel.border(border_style::rounded);
            content_panel.key("content-panel");
            layout.add(std::move(content_panel), 1);
        }

        layout.gap(1);
        layout.key("sidebar-layout");
        con.print_widget(std::move(layout), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  9. Interactive Widgets with Keys
// ═══════════════════════════════════════════════════════════════════════

static void demo_interactive_keys(console& con) {
    section(con, " 9. Interactive Widgets ");

    subsection(con, "9a. Button with key and style:");
    con.print_widget(
        button_builder("Submit")
            .key("submit-btn")
            .style_override(style{colors::bright_white, colors::blue, attr::bold})
            .focused(true),
        20);

    con.newline();

    subsection(con, "9b. Checkbox with key:");
    {
        bool checked = true;
        con.print_widget(
            checkbox_builder("Enable notifications", &checked)
                .key("notif-cb"),
            35);
    }

    con.newline();

    subsection(con, "9c. Slider with key:");
    {
        float val = 0.7f;
        con.print_widget(
            slider_builder(&val, 0.0f, 1.0f)
                .width(30)
                .show_percentage(true)
                .key("volume-slider"),
            45);
    }

    con.newline();

    subsection(con, "9d. Text input with key:");
    {
        std::string buf = "Hello, tapiru!";
        con.print_widget(
            text_input_builder(&buf)
                .width(30)
                .placeholder("Type here...")
                .focused(true)
                .cursor_pos(14)
                .key("main-input"),
            40);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  10. Live Dashboard (animated)
// ═══════════════════════════════════════════════════════════════════════

static void demo_live_dashboard(console& con) {
    section(con, " 10. Live Dashboard (animated) ");

    if (!terminal::is_tty()) {
        con.print("[yellow]Skipped: requires interactive terminal (TTY)[/]");
        con.print("[dim]Run directly in Windows Terminal to see the live animation.[/]");
        return;
    }

    con.print("[dim]Running a 3-second live dashboard animation...[/]");
    con.newline();

    auto cpu_task  = std::make_shared<progress_task>("CPU",    100);
    auto mem_task  = std::make_shared<progress_task>("Memory", 100);
    auto disk_task = std::make_shared<progress_task>("Disk",   100);

    {
        live lv(con, 12, W);

        for (int frame = 0; frame <= 30; ++frame) {
            double t = frame * 0.1;
            int cpu  = static_cast<int>(50 + 30 * std::sin(t * 2.0));
            int mem  = static_cast<int>(40 + 10 * std::sin(t * 0.7 + 1.0));
            int disk = std::min(100, frame * 3 + 5);

            cpu_task->set_current(cpu);
            mem_task->set_current(mem);
            disk_task->set_current(disk);

            rows_builder dashboard;

            dashboard.add(
                text_builder("[bold bright_white on_blue]  Live System Monitor  [/]")
            );

            {
                columns_builder body;
                body.add(
                    progress_builder()
                        .add_task(cpu_task)
                        .add_task(mem_task)
                        .add_task(disk_task)
                        .bar_width(25)
                        .key("live-progress"),
                    1
                );
                body.gap(1);
                body.key("live-body");
                dashboard.add(std::move(body));
            }

            char status_buf[64];
            std::snprintf(status_buf, sizeof(status_buf), "Frame %d/30", frame);

            dashboard.add(
                status_bar_builder()
                    .left("[bold] LIVE [/]")
                    .center(status_buf)
                    .right(disk >= 100 ? "[green]Complete[/]" : "[yellow]Running...[/]")
                    .style_override(style{colors::bright_white, color::from_rgb(30, 30, 60)})
            );

            dashboard.gap(0);
            dashboard.key("live-dashboard");
            lv.set(std::move(dashboard));

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    con.newline();
    con.print("[bold green]Live dashboard finished![/]");
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    console con;

    con.newline();
    con.print_widget(
        rule_builder(" tapiru — New Features Comprehensive Demo ")
            .rule_style(style{colors::bright_yellow, {}, attr::bold})
            .character(U'\x2550'),  // ═
        W);

    con.newline();
    con.print("[dim]This demo showcases all features added in the widget expansion:[/]");
    con.print("[dim]  rows_builder, spacer_builder, status_bar_builder, menu_builder,[/]");
    con.print("[dim]  popup_builder, table shadow/glow/gradient, container opacity,[/]");
    con.print("[dim]  widget keys (.key()), z_order, and interactive widget support.[/]");

    demo_rows_basic(con);
    demo_spacer(con);
    demo_status_bar(con);
    demo_menu(con);
    demo_popup(con);
    demo_table_visuals(con);
    demo_widget_keys(con);
    demo_complex_layout(con);
    demo_interactive_keys(con);
    demo_live_dashboard(con);

    con.newline();
    con.print_widget(
        rule_builder(" New Features Demo Complete! ")
            .rule_style(style{colors::bright_green, {}, attr::bold})
            .character(U'\x2550'),  // ═
        W);
    con.newline();

    return 0;
}
