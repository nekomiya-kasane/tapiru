/**
 * @file widgets_demo.cpp
 * @brief Comprehensive demo of the latest tapiru widget expansion.
 *
 * Covers: sized_box, center, scroll_view, multi-level menu, menu_bar,
 *         tab, transition animations, accordion, tree_view, dropdown,
 *         toggle, toast, textarea, log_viewer, search_input, breadcrumb,
 *         keybinding, grid.
 *
 * Build:
 *   cmake --build build --target tapiru_widgets_demo --config Release
 * Run:
 *   .\build\bin\Release\tapiru_widgets_demo.exe
 */

#include <chrono>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

// ── Core ────────────────────────────────────────────────────────────────
#include "tapiru/core/console.h"
#include "tapiru/core/style.h"
#include "tapiru/core/transition.h"

// ── Widgets ─────────────────────────────────────────────────────────────
#include "tapiru/widgets/accordion.h"
#include "tapiru/widgets/breadcrumb.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/dropdown.h"
#include "tapiru/widgets/grid.h"
#include "tapiru/widgets/keybinding.h"
#include "tapiru/widgets/log_viewer.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/menu_bar.h"
#include "tapiru/widgets/scroll_view.h"
#include "tapiru/widgets/search_input.h"
#include "tapiru/widgets/tab.h"
#include "tapiru/widgets/textarea.h"
#include "tapiru/widgets/toast.h"
#include "tapiru/widgets/toggle.h"
#include "tapiru/widgets/tree_view.h"

using namespace tapiru;

static constexpr uint32_t W = 76;

// ═══════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════

static void section(console &con, const char *title) {
    con.newline();
    con.print_widget(rule_builder(title).rule_style(style{colors::bright_cyan, {}, attr::bold}), W);
    con.newline();
}

static void subsection(console &con, const char *desc) {
    con.print(std::string("[dim]") + desc + "[/]");
}

// ═══════════════════════════════════════════════════════════════════════
//  1. Sized Box & Center
// ═══════════════════════════════════════════════════════════════════════

static void demo_sized_box_center(console &con) {
    section(con, " 1. Sized Box & Center ");

    subsection(con, "1a. sized_box: fixed 40x3 box containing text:");
    {
        auto sb = sized_box_builder(text_builder("[bold cyan]Fixed size content[/]"));
        sb.width(40).height(3);
        auto panel = panel_builder(std::move(sb));
        panel.border(border_style::rounded);
        panel.border_style_override(style{colors::bright_black});
        con.print_widget(std::move(panel), 44);
    }

    con.newline();

    subsection(con, "1b. center: text centered in a 50-wide box:");
    {
        auto cb = center_builder(text_builder("[bold yellow]Centered![/]"));
        auto sb = sized_box_builder(std::move(cb));
        sb.width(50).height(3);
        auto panel = panel_builder(std::move(sb));
        panel.border(border_style::rounded);
        con.print_widget(std::move(panel), 54);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  2. Scroll View
// ═══════════════════════════════════════════════════════════════════════

static void demo_scroll_view(console &con) {
    section(con, " 2. Scroll View ");

    subsection(con, "Scrollable viewport (5 visible lines, scrolled to line 3):");
    {
        rows_builder content;
        for (int i = 0; i < 20; ++i) {
            content.add(text_builder("  Line " + std::to_string(i + 1) + ": Lorem ipsum dolor sit amet"));
        }

        int scroll_y = 3;
        auto sv = scroll_view_builder(std::move(content));
        sv.viewport_width(50).viewport_height(5);
        sv.scroll_y(&scroll_y);
        sv.show_scrollbar_v(true);
        sv.border(border_style::rounded);
        con.print_widget(std::move(sv), 55);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  3. Multi-Level Menu
// ═══════════════════════════════════════════════════════════════════════

static void demo_multilevel_menu(console &con) {
    section(con, " 3. Multi-Level Menu ");

    subsection(con, "3a. Menu with nested submenu items:");
    {
        int cursor = 1;
        auto mb = menu_builder();
        mb.add(menu_item_builder("New File").shortcut("Ctrl+N"));
        mb.add(menu_item_builder("Open Recent")
                   .add(menu_item_builder("project.cpp"))
                   .add(menu_item_builder("main.h"))
                   .add(menu_item_builder("CMakeLists.txt")));
        mb.add_separator();
        mb.add(menu_item_builder("Save").shortcut("Ctrl+S"));
        mb.add(menu_item_builder("Exit").shortcut("Ctrl+Q"));
        mb.cursor(&cursor);
        mb.border(border_style::rounded);
        mb.highlight_style(style{colors::bright_white, colors::blue, attr::bold});
        mb.shortcut_style(style{colors::bright_black});
        mb.shadow();
        con.print_widget(std::move(mb), 35);
    }

    con.newline();

    subsection(con, "3b. Menu with disabled and checked items:");
    {
        int cursor = 2;
        auto mb = menu_builder();
        mb.add(menu_item_builder("Undo").shortcut("Ctrl+Z").disabled(true));
        mb.add(menu_item_builder("Redo").shortcut("Ctrl+Y").disabled(true));
        mb.add(menu_item_builder("Word Wrap").checked(true));
        mb.add(menu_item_builder("Line Numbers").checked(true));
        mb.add(menu_item_builder("Minimap").checked(false));
        mb.cursor(&cursor);
        mb.border(border_style::rounded);
        mb.highlight_style(style{colors::bright_white, colors::green, attr::bold});
        con.print_widget(std::move(mb), 30);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  4. Menu Bar
// ═══════════════════════════════════════════════════════════════════════

static void demo_menu_bar(console &con) {
    section(con, " 4. Menu Bar ");

    subsection(con, "Horizontal menu bar (File menu active):");
    {
        menu_bar_state state;
        state.activate(0);

        auto bar = menu_bar_builder();
        bar.add_menu("File", {
                                 menu_item_builder("New").shortcut("Ctrl+N"),
                                 menu_item_builder("Open").shortcut("Ctrl+O"),
                                 menu_item_builder("Exit").shortcut("Ctrl+Q"),
                             });
        bar.add_menu("Edit", {
                                 menu_item_builder("Undo").shortcut("Ctrl+Z"),
                                 menu_item_builder("Redo").shortcut("Ctrl+Y"),
                             });
        bar.add_menu("View", {
                                 menu_item_builder("Zoom In").shortcut("Ctrl++"),
                                 menu_item_builder("Zoom Out").shortcut("Ctrl+-"),
                             });
        bar.state(&state);
        bar.bar_style(style{colors::white, color::from_rgb(40, 40, 60)});
        bar.active_style(style{colors::bright_white, colors::blue, attr::bold});
        con.print_widget(std::move(bar), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  5. Tab Widget
// ═══════════════════════════════════════════════════════════════════════

static void demo_tab(console &con) {
    section(con, " 5. Tab Widget ");

    subsection(con, "Tab view with 3 tabs (second tab active):");
    {
        int active = 1;
        auto tabs = tab_builder();
        tabs.add_tab("General", text_builder("  [bold]Application Name:[/] tapiru\n"
                                             "  [bold]Version:[/]          0.2.0\n"
                                             "  [bold]License:[/]          MIT"));
        tabs.add_tab("Editor", text_builder("  [bold]Font:[/]       Cascadia Code\n"
                                            "  [bold]Size:[/]       14px\n"
                                            "  [bold]Theme:[/]      Dark Modern\n"
                                            "  [bold]Tab Size:[/]   4 spaces"));
        tabs.add_tab("Keybindings", text_builder("  [bold]Ctrl+S[/]  Save\n"
                                                 "  [bold]Ctrl+Z[/]  Undo\n"
                                                 "  [bold]Ctrl+P[/]  Command Palette"));
        tabs.active(&active);
        tabs.tab_style(style{colors::bright_black, color::from_rgb(30, 30, 40)});
        tabs.active_tab_style(style{colors::bright_white, colors::blue, attr::bold});
        tabs.content_border(border_style::rounded);
        con.print_widget(std::move(tabs), 50);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  6. Transition Animations
// ═══════════════════════════════════════════════════════════════════════

static void demo_transitions(console &con) {
    section(con, " 6. Transition Animations ");

    subsection(con, "6a. Typewriter effect (snapshot at 60% progress):");
    {
        typewriter tw("The quick brown fox jumps over the lazy dog.", duration_ms(2000));
        tw.start(steady_clock::now() - duration_ms(1200)); // 60% through
        std::string visible = tw.value(steady_clock::now());
        con.print_widget(text_builder("[bold green]> [/]" + visible + "[dim]|[/]"), W);
    }

    con.newline();

    subsection(con, "6b. Counter animation (counting from 0 to 1000, at 75%):");
    {
        counter_animation ca(0.0, 1000.0, duration_ms(1000), easing::ease_out);
        ca.start(steady_clock::now() - duration_ms(750));
        int val = ca.int_value(steady_clock::now());
        con.print_widget(text_builder("[bold bright_yellow]Downloads: [/][bold]" + std::to_string(val) + "[/]"), W);
    }

    con.newline();

    subsection(con, "6c. Marquee scrolling text:");
    {
        marquee mq("  Breaking News: tapiru v0.2 released with 17 new widgets!  ", 40, duration_ms(150));
        mq.start(steady_clock::now() - duration_ms(3000));
        std::string visible = mq.value(steady_clock::now());
        auto panel = panel_builder(text_builder("[bold cyan]" + visible + "[/]"));
        panel.border(border_style::rounded);
        con.print_widget(std::move(panel), 44);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  7. Accordion
// ═══════════════════════════════════════════════════════════════════════

static void demo_accordion(console &con) {
    section(con, " 7. Accordion ");

    subsection(con, "Collapsible sections (first and third expanded):");
    {
        std::vector<bool> expanded = {true, false, true};
        auto acc = accordion_builder();
        acc.add_section("Getting Started", text_builder("    Install tapiru via CMake:\n"
                                                        "    [cyan]add_subdirectory(tapiru)[/]\n"
                                                        "    [cyan]target_link_libraries(app PRIVATE tapiru)[/]"));
        acc.add_section("API Reference", text_builder("    (collapsed — click to expand)"));
        acc.add_section("Examples", text_builder("    See the [bold]examples/[/] directory for demos:\n"
                                                 "    [dim]demo.cpp, interactive_demo.cpp, widgets_demo.cpp[/]"));
        acc.expanded(&expanded);
        acc.header_style(style{colors::bright_white, color::from_rgb(40, 40, 60), attr::bold});
        acc.active_header_style(style{colors::bright_cyan, color::from_rgb(20, 40, 80), attr::bold});
        con.print_widget(std::move(acc), 55);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  8. Tree View
// ═══════════════════════════════════════════════════════════════════════

static void demo_tree_view(console &con) {
    section(con, " 8. Tree View ");

    subsection(con, "File tree with expanded directories:");
    {
        tree_node root;
        root.label = "tapiru/";
        root.children = {
            {"include/",
             {
                 {"tapiru/",
                  {
                      {"core/", {{"style.h", {}}, {"canvas.h", {}}, {"animation.h", {}}}},
                      {"widgets/", {{"builders.h", {}}, {"menu.h", {}}, {"tab.h", {}}}},
                  }},
             }},
            {"src/",
             {
                 {"detail/", {{"scene.h", {}}, {"widgets.cpp", {}}}},
                 {"widgets/", {{"menu.cpp", {}}, {"tab.cpp", {}}}},
             }},
            {"tests/", {{"widget_render_test.cpp", {}}}},
            {"CMakeLists.txt", {}},
        };

        std::unordered_set<std::string> expanded = {"tapiru/", "include/", "tapiru/", "core/", "src/"};
        int cursor = 3;

        auto tv = tree_view_builder();
        tv.root(std::move(root));
        tv.expanded_set(&expanded);
        tv.cursor(&cursor);
        tv.node_style(style{colors::white});
        tv.highlight_style(style{colors::bright_white, colors::blue, attr::bold});
        con.print_widget(std::move(tv), 50);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  9. Dropdown
// ═══════════════════════════════════════════════════════════════════════

static void demo_dropdown(console &con) {
    section(con, " 9. Dropdown ");

    subsection(con, "9a. Closed dropdown (selected: Dark):");
    {
        int sel = 0;
        bool open = false;
        auto dd = dropdown_builder();
        dd.add_option("Dark").add_option("Light").add_option("Monokai").add_option("Solarized");
        dd.selected(&sel);
        dd.open(&open);
        dd.button_style(style{colors::bright_white, color::from_rgb(40, 40, 60)});
        dd.border(border_style::rounded);
        con.print_widget(std::move(dd), 25);
    }

    con.newline();

    subsection(con, "9b. Open dropdown (cursor on Monokai):");
    {
        int sel = 2;
        bool open = true;
        auto dd = dropdown_builder();
        dd.add_option("Dark").add_option("Light").add_option("Monokai").add_option("Solarized");
        dd.selected(&sel);
        dd.open(&open);
        dd.button_style(style{colors::bright_white, color::from_rgb(40, 40, 60)});
        dd.highlight_style(style{colors::bright_white, colors::blue, attr::bold});
        dd.border(border_style::rounded);
        con.print_widget(std::move(dd), 25);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  10. Toggle
// ═══════════════════════════════════════════════════════════════════════

static void demo_toggle(console &con) {
    section(con, " 10. Toggle Switch ");

    subsection(con, "Toggle switches with different states:");
    {
        bool on = true;
        bool off = false;

        rows_builder rb;
        rb.add(toggle_builder()
                   .value(&on)
                   .label("Dark Mode")
                   .on_style(style{colors::bright_green, {}, attr::bold})
                   .off_style(style{colors::bright_black})
                   .label_style(style{colors::white}));
        rb.add(toggle_builder()
                   .value(&off)
                   .label("Auto Save")
                   .on_style(style{colors::bright_green, {}, attr::bold})
                   .off_style(style{colors::bright_black})
                   .label_style(style{colors::white}));
        rb.add(toggle_builder()
                   .value(&on)
                   .label("Line Numbers")
                   .on_style(style{colors::bright_cyan, {}, attr::bold})
                   .off_style(style{colors::bright_black})
                   .label_style(style{colors::white}));
        rb.gap(0);
        con.print_widget(std::move(rb), 30);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  11. Toast
// ═══════════════════════════════════════════════════════════════════════

static void demo_toast(console &con) {
    section(con, " 11. Toast Notifications ");

    subsection(con, "Toast messages at different levels:");
    {
        rows_builder rb;

        toast_state ts_info;
        ts_info.show("File saved successfully.", toast_level::info);
        rb.add(toast_builder().state(&ts_info).info_style(style{colors::bright_cyan}).border(border_style::rounded));

        toast_state ts_success;
        ts_success.show("Build completed: 0 errors.", toast_level::success);
        rb.add(toast_builder()
                   .state(&ts_success)
                   .success_style(style{colors::bright_green})
                   .border(border_style::rounded));

        toast_state ts_warn;
        ts_warn.show("Disk space running low.", toast_level::warning);
        rb.add(
            toast_builder().state(&ts_warn).warning_style(style{colors::bright_yellow}).border(border_style::rounded));

        toast_state ts_err;
        ts_err.show("Connection refused: ECONNREFUSED.", toast_level::error);
        rb.add(toast_builder().state(&ts_err).error_style(style{colors::bright_red}).border(border_style::rounded));

        rb.gap(0);
        con.print_widget(std::move(rb), 50);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  12. Textarea
// ═══════════════════════════════════════════════════════════════════════

static void demo_textarea(console &con) {
    section(con, " 12. Textarea ");

    subsection(con, "Multi-line text editor with line numbers:");
    {
        std::string code = "#include <iostream>\n"
                           "\n"
                           "int main() {\n"
                           "    std::cout << \"Hello, tapiru!\" << std::endl;\n"
                           "    return 0;\n"
                           "}\n";

        int cursor_row = 3;
        int cursor_col = 0;
        int scroll_row = 0;

        auto ta = textarea_builder();
        ta.text(&code);
        ta.cursor_row(&cursor_row);
        ta.cursor_col(&cursor_col);
        ta.scroll_row(&scroll_row);
        ta.width(50);
        ta.height(6);
        ta.show_line_numbers(true);
        ta.text_style(style{colors::white});
        ta.cursor_style(style{colors::bright_white, color::from_rgb(40, 40, 80)});
        ta.border(border_style::rounded);
        ta.border_style_override(style{colors::bright_black});
        con.print_widget(std::move(ta), 60);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  13. Log Viewer
// ═══════════════════════════════════════════════════════════════════════

static void demo_log_viewer(console &con) {
    section(con, " 13. Log Viewer ");

    subsection(con, "Scrollable log viewer with level filtering:");
    {
        std::vector<log_entry> logs = {
            {log_level::info, "10:00:01", "Application started"},
            {log_level::info, "10:00:02", "Loading configuration..."},
            {log_level::warn, "10:00:03", "Config key 'theme' not found, using default"},
            {log_level::info, "10:00:04", "Server listening on port 8080"},
            {log_level::error, "10:00:05", "Failed to connect to database"},
            {log_level::info, "10:00:06", "Retrying connection..."},
            {log_level::info, "10:00:07", "Database connected successfully"},
            {log_level::debug, "10:00:08", "Query executed in 12ms"},
        };
        int scroll = 0;

        auto lv = log_viewer_builder();
        lv.entries(&logs);
        lv.scroll(&scroll);
        lv.height(6);
        lv.show_timestamp(true);
        lv.show_level(true);
        lv.border(border_style::rounded);
        con.print_widget(std::move(lv), 65);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  14. Search Input
// ═══════════════════════════════════════════════════════════════════════

static void demo_search_input(console &con) {
    section(con, " 14. Search Input ");

    subsection(con, "Search field with match count:");
    {
        std::string query = "widget";
        int matches = 17;
        int current = 4;

        auto si = search_input_builder();
        si.query(&query);
        si.match_count(&matches);
        si.current_match(&current);
        si.width(30);
        si.input_style(style{colors::bright_white});
        si.match_style(style{colors::bright_yellow});
        si.border(border_style::rounded);
        con.print_widget(std::move(si), 45);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  15. Breadcrumb
// ═══════════════════════════════════════════════════════════════════════

static void demo_breadcrumb(console &con) {
    section(con, " 15. Breadcrumb ");

    subsection(con, "Navigation breadcrumb trail:");
    {
        int active = 3;
        auto bc = breadcrumb_builder();
        bc.add_item("Home");
        bc.add_item("Projects");
        bc.add_item("tapiru");
        bc.add_item("src");
        bc.active(&active);
        bc.item_style(style{colors::bright_blue, {}, attr::underline});
        bc.active_style(style{colors::bright_white, {}, attr::bold});
        bc.separator_style(style{colors::bright_black});
        con.print_widget(std::move(bc), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  16. Keybinding Display
// ═══════════════════════════════════════════════════════════════════════

static void demo_keybinding(console &con) {
    section(con, " 16. Keybinding Display ");

    subsection(con, "Keyboard shortcuts reference:");
    {
        auto kb = keybinding_builder();
        kb.add("Ctrl+S", "Save file");
        kb.add("Ctrl+Z", "Undo");
        kb.add("Ctrl+Shift+P", "Command palette");
        kb.add("Ctrl+`", "Toggle terminal");
        kb.add("F5", "Start debugging");
        kb.add("Ctrl+B", "Toggle sidebar");
        kb.key_style(style{colors::bright_cyan, color::from_rgb(30, 30, 50), attr::bold});
        kb.desc_style(style{colors::white});
        con.print_widget(std::move(kb), 50);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  17. Grid Layout
// ═══════════════════════════════════════════════════════════════════════

static void demo_grid(console &con) {
    section(con, " 17. Grid Layout ");

    subsection(con, "2x3 grid of status cards:");
    {
        auto card = [](const char *title, const char *value, const style &sty) {
            auto panel = panel_builder(text_builder(std::string("[bold]") + title + "[/]\n" + value));
            panel.border(border_style::rounded);
            panel.border_style_override(sty);
            return panel;
        };

        auto g = grid_builder();
        g.columns(3);
        g.col_gap(1);
        g.row_gap(0);
        g.add(card("CPU", "[green]23%[/]", style{colors::green}));
        g.add(card("Memory", "[yellow]67%[/]", style{colors::yellow}));
        g.add(card("Disk", "[cyan]45%[/]", style{colors::cyan}));
        g.add(card("Network", "[blue]12 MB/s[/]", style{colors::blue}));
        g.add(card("GPU", "[red]89%[/]", style{colors::red}));
        g.add(card("Uptime", "[white]3d 14h[/]", style{colors::white}));
        con.print_widget(std::move(g), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  18. Composite Dashboard
// ═══════════════════════════════════════════════════════════════════════

static void demo_composite(console &con) {
    section(con, " 18. Composite Dashboard ");

    subsection(con, "Combining multiple new widgets into a dashboard:");
    {
        rows_builder dash;

        // Breadcrumb navigation
        {
            int active = 2;
            dash.add(breadcrumb_builder()
                         .add_item("Dashboard")
                         .add_item("Monitoring")
                         .add_item("Overview")
                         .active(&active)
                         .item_style(style{colors::bright_blue})
                         .active_style(style{colors::bright_white, {}, attr::bold})
                         .separator_style(style{colors::bright_black}));
        }

        // Toggle row
        {
            bool dark = true;
            bool notif = false;
            columns_builder toggles;
            toggles.add(toggle_builder()
                            .value(&dark)
                            .label("Dark Mode")
                            .on_style(style{colors::bright_green, {}, attr::bold})
                            .off_style(style{colors::bright_black}));
            toggles.add(spacer_builder(), 1);
            toggles.add(toggle_builder()
                            .value(&notif)
                            .label("Notifications")
                            .on_style(style{colors::bright_green, {}, attr::bold})
                            .off_style(style{colors::bright_black}));
            toggles.gap(0);
            dash.add(std::move(toggles));
        }

        // Toast notification
        {
            toast_state ts;
            ts.show("System health: all services operational.", toast_level::success);
            dash.add(
                toast_builder().state(&ts).success_style(style{colors::bright_green}).border(border_style::rounded));
        }

        // Keybindings footer
        {
            columns_builder footer;
            footer.add(text_builder("[dim][Ctrl+R] Refresh[/]"));
            footer.add(spacer_builder(), 1);
            footer.add(text_builder("[dim][Ctrl+Q] Quit[/]"));
            footer.gap(0);
            dash.add(std::move(footer));
        }

        dash.gap(1);
        auto panel = panel_builder(std::move(dash));
        panel.title("System Dashboard");
        panel.border(border_style::rounded);
        panel.border_style_override(style{colors::bright_cyan});
        con.print_widget(std::move(panel), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    console con;

    con.newline();
    con.print_widget(rule_builder(" tapiru \xe2\x80\x94 Widget Expansion Demo ")
                         .rule_style(style{colors::bright_yellow, {}, attr::bold})
                         .character(U'\x2550'),
                     W);

    con.newline();
    con.print("[dim]Showcasing 17 new widgets and features added to tapiru:[/]");
    con.print("[dim]  sized_box, center, scroll_view, multi-level menu, menu_bar,[/]");
    con.print("[dim]  tab, transitions, accordion, tree_view, dropdown, toggle,[/]");
    con.print("[dim]  toast, textarea, log_viewer, search_input, breadcrumb,[/]");
    con.print("[dim]  keybinding, grid.[/]");

    demo_sized_box_center(con);
    demo_scroll_view(con);
    demo_multilevel_menu(con);
    demo_menu_bar(con);
    demo_tab(con);
    demo_transitions(con);
    demo_accordion(con);
    demo_tree_view(con);
    demo_dropdown(con);
    demo_toggle(con);
    demo_toast(con);
    demo_textarea(con);
    demo_log_viewer(con);
    demo_search_input(con);
    demo_breadcrumb(con);
    demo_keybinding(con);
    demo_grid(con);
    demo_composite(con);

    con.newline();
    con.print_widget(rule_builder(" Widget Expansion Demo Complete! ")
                         .rule_style(style{colors::bright_green, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    return 0;
}
