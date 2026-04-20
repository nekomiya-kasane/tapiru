/**
 * @file interactive_demo.cpp
 * @brief Interactive TUI demo showcasing all new tapiru widget features.
 *
 * Multi-page demo with keyboard navigation:
 *   Page 0: Menu & Popup     — navigate menu, trigger popups
 *   Page 1: Status Bars      — cycle through status bar styles
 *   Page 2: Rows & Spacer    — adjust gap, toggle spacer modes
 *   Page 3: Table Visuals    — shadow / glow / gradient toggle
 *   Page 4: Dashboard        — composite layout with all new widgets
 *   Page 5: tqdm Demo        — run progress bar iterations
 *
 * Controls:
 *   Tab / Shift+Tab  — switch pages
 *   Up / Down        — navigate within page
 *   Enter / Space    — toggle / activate
 *   Left / Right     — adjust values
 *   q / Escape       — quit
 *
 * Build:
 *   cmake --build build --target tapiru_interactive_demo --config Debug
 * Run:
 *   .\build\bin\Debug\tapiru_interactive_demo.exe
 */

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <Windows.h>
#else
#  include <termios.h>
#  include <unistd.h>
#  include <sys/select.h>
#endif

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "tapiru/core/console.h"
#include "tapiru/core/gradient.h"
#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"
#include "tapiru/core/tqdm.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/interactive.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/popup.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/status_bar.h"

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
//  Input abstraction (same pattern as tui_demo.cpp)
// ═══════════════════════════════════════════════════════════════════════

enum class input_ev : uint8_t {
    none, up, down, left, right, enter, tab, shift_tab, quit, space,
    mouse_click  // mouse left-click (coordinates stored separately)
};

static int g_mouse_x = 0;
static int g_mouse_y = 0;

#ifdef _WIN32

static HANDLE g_stdin = INVALID_HANDLE_VALUE;
static DWORD  g_old_mode = 0;

static void enable_raw_input() {
    g_stdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(g_stdin, &g_old_mode);
    SetConsoleMode(g_stdin, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT
                   | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);
}

static void restore_input() {
    if (g_stdin != INVALID_HANDLE_VALUE) SetConsoleMode(g_stdin, g_old_mode);
}

static input_ev poll_input(int timeout_ms) {
    if (WaitForSingleObject(g_stdin, static_cast<DWORD>(timeout_ms)) != WAIT_OBJECT_0)
        return input_ev::none;
    INPUT_RECORD rec;
    DWORD count = 0;
    if (!ReadConsoleInputW(g_stdin, &rec, 1, &count) || count == 0)
        return input_ev::none;
    if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
        auto vk = rec.Event.KeyEvent.wVirtualKeyCode;
        auto ch = rec.Event.KeyEvent.uChar.UnicodeChar;
        bool shift = (rec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) != 0;
        if (vk == VK_UP)     return input_ev::up;
        if (vk == VK_DOWN)   return input_ev::down;
        if (vk == VK_LEFT)   return input_ev::left;
        if (vk == VK_RIGHT)  return input_ev::right;
        if (vk == VK_RETURN) return input_ev::enter;
        if (vk == VK_TAB)    return shift ? input_ev::shift_tab : input_ev::tab;
        if (vk == VK_ESCAPE) return input_ev::quit;
        if (ch == L'q' || ch == L'Q') return input_ev::quit;
        if (ch == L' ')      return input_ev::space;
    }
    if (rec.EventType == MOUSE_EVENT) {
        auto& me = rec.Event.MouseEvent;
        if (me.dwEventFlags == 0 && (me.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
            g_mouse_x = me.dwMousePosition.X;
            g_mouse_y = me.dwMousePosition.Y;
            return input_ev::mouse_click;
        }
    }
    return input_ev::none;
}

#else

static struct termios g_old_termios;

static void enable_raw_input() {
    tcgetattr(STDIN_FILENO, &g_old_termios);
    struct termios raw = g_old_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void restore_input() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_old_termios);
}

static input_ev poll_input(int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
        return input_ev::none;
    char buf[16];
    int n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return input_ev::none;
    if (n == 1) {
        if (buf[0] == 'q' || buf[0] == 'Q' || buf[0] == 27) return input_ev::quit;
        if (buf[0] == '\n' || buf[0] == '\r') return input_ev::enter;
        if (buf[0] == ' ') return input_ev::space;
        if (buf[0] == '\t') return input_ev::tab;
    } else if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
        if (buf[2] == 'A') return input_ev::up;
        if (buf[2] == 'B') return input_ev::down;
        if (buf[2] == 'C') return input_ev::right;
        if (buf[2] == 'D') return input_ev::left;
        if (buf[2] == 'Z') return input_ev::shift_tab;
    }
    return input_ev::none;
}

#endif

// ═══════════════════════════════════════════════════════════════════════
//  Application State
// ═══════════════════════════════════════════════════════════════════════

static constexpr int NUM_PAGES = 6;
static const char* page_names[NUM_PAGES] = {
    "Menu", "StatusBar", "Rows", "Table", "Dashboard", "tqdm"
};

struct app_state {
    bool quit = false;
    int  page = 0;
    int  cursor = 0;
    int  tick = 0;

    // Page 0: Menu & Popup
    int  menu_cursor = 0;
    bool show_popup = false;
    int  popup_item = -1;  // which menu item triggered the popup

    // Page 1: Status Bar
    int  sb_style = 0;  // 0=editor, 1=build, 2=error, 3=minimal

    // Page 2: Rows & Spacer
    int  row_gap = 0;
    int  spacer_mode = 0;  // 0=left-right, 1=centered, 2=three-section

    // Page 3: Table Visuals
    int  table_effect = 0;  // 0=none, 1=shadow, 2=glow, 3=custom shadow

    // Page 4: Dashboard (composite)
    int  dash_cursor = 0;

    // Page 5: tqdm
    bool tqdm_running = false;
    int  tqdm_progress = 0;
    int  tqdm_total = 50;

    // Mouse tracking (shown on all pages)
    int  last_click_x = -1;
    int  last_click_y = -1;
    int  click_count = 0;
};

// Menu separator indices (items at these positions are separators, not selectable)
static bool is_menu_separator(int idx) {
    return idx == 3 || idx == 6;
}

static int menu_cursor_next(int cur) {
    int next = cur + 1;
    if (next > 7) return cur;
    if (is_menu_separator(next)) ++next;
    if (next > 7) return cur;
    return next;
}

static int menu_cursor_prev(int cur) {
    int prev = cur - 1;
    if (prev < 0) return cur;
    if (is_menu_separator(prev)) --prev;
    if (prev < 0) return cur;
    return prev;
}

// ═══════════════════════════════════════════════════════════════════════
//  Page views — each builds a widget tree
// ═══════════════════════════════════════════════════════════════════════

static auto build_tab_bar(const app_state& st) {
    std::string tabs = "  ";
    for (int i = 0; i < NUM_PAGES; ++i) {
        if (i == st.page)
            tabs += std::string("[bold bright_white on_blue] ") + page_names[i] + " [/]";
        else
            tabs += std::string("[dim] ") + page_names[i] + " [/]";
        if (i < NUM_PAGES - 1) tabs += " ";
    }
    return text_builder(tabs);
}

static auto build_footer(const app_state& st) {
    std::string footer =
        "  [dim]Tab[/] page  [dim]\xe2\x86\x91\xe2\x86\x93[/] navigate  "
        "[dim]Enter[/] select  [dim]\xe2\x86\x90\xe2\x86\x92[/] adjust  "
        "[dim]Mouse[/] click tabs  [dim]q[/] quit";
    if (st.click_count > 0) {
        char buf[64];
        std::snprintf(buf, sizeof(buf),
            "\n  [dim]Last click:[/] (%d, %d)  [dim]Total clicks:[/] %d",
            st.last_click_x, st.last_click_y, st.click_count);
        footer += buf;
    }
    return text_builder(footer);
}

// ── Page 0: Menu & Popup ────────────────────────────────────────────────

class menu_page_view {
public:
    explicit menu_page_view(const app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Menu & Popup Demo[/]"));
        page.add(text_builder(""));

        // Two columns: menu on left, info on right
        columns_builder body;

        // Left: menu
        {
            auto mb = menu_builder()
                .add_item("New Project",    "Ctrl+N")
                .add_item("Open Project",   "Ctrl+O")
                .add_item("Save All",       "Ctrl+Shift+S")
                .add_separator()
                .add_item("Settings",       "Ctrl+,")
                .add_item("Extensions",     "Ctrl+Shift+X")
                .add_separator()
                .add_item("Exit",           "Ctrl+Q")
                .cursor(&st_.menu_cursor)
                .border(border_style::rounded)
                .highlight_style(style{colors::bright_white, colors::blue, attr::bold})
                .shortcut_style(style{colors::bright_black})
                .shadow()
                .key("main-menu");
            body.add(std::move(mb));
        }

        // Right: description panel
        {
            const char* descriptions[] = {
                "[bold]New Project[/]\nCreate a new project from template.",
                "[bold]Open Project[/]\nBrowse and open an existing project.",
                "[bold]Save All[/]\nSave all modified files to disk.",
                "",  // separator
                "[bold]Settings[/]\nConfigure editor preferences.",
                "[bold]Extensions[/]\nManage installed extensions.",
                "",  // separator
                "[bold]Exit[/]\nClose the application.",
            };

            int desc_idx = st_.menu_cursor;
            const char* desc_text = (desc_idx >= 0 && desc_idx < 8)
                ? descriptions[desc_idx] : "";

            auto info_text = text_builder(desc_text);
            panel_builder info(std::move(info_text));
            info.title("Details");
            info.border(border_style::rounded);
            info.key("info-panel");
            body.add(std::move(info), 1);
        }

        body.gap(2);
        body.key("menu-body");
        page.add(std::move(body));

        page.add(text_builder(""));
        page.add(text_builder("  [dim]Press Enter to show popup for selected item[/]"));

        // Popup overlay (if active)
        if (st_.show_popup && st_.popup_item >= 0) {
            const char* item_names[] = {
                "New Project", "Open Project", "Save All", "",
                "Settings", "Extensions", "", "Exit"
            };
            const char* name = (st_.popup_item < 8) ? item_names[st_.popup_item] : "?";

            std::string popup_text = std::string("[bold]") + name + "[/]\n\n"
                "This action would be executed.\n\n"
                "[dim]Press Enter or Esc to dismiss.[/]";

            page.add(text_builder(""));

            auto popup_content = text_builder(popup_text);
            panel_builder popup_panel(std::move(popup_content));
            popup_panel.title("Action");
            popup_panel.border(border_style::rounded);
            popup_panel.border_style_override(style{colors::bright_yellow});
            popup_panel.shadow();
            popup_panel.key("action-popup");
            page.add(std::move(popup_panel));
        }

        page.key("menu-page");
        return page.flatten_into(s);
    }
private:
    const app_state& st_;
};

// ── Page 1: Status Bars ─────────────────────────────────────────────────

class statusbar_page_view {
public:
    explicit statusbar_page_view(const app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Status Bar Styles[/]"));
        page.add(text_builder("  [dim]Use Up/Down to cycle styles[/]"));
        page.add(text_builder(""));

        const char* style_names[] = {"Editor", "Build", "Error", "Minimal"};
        for (int i = 0; i < 4; ++i) {
            bool sel = (st_.sb_style == i);
            std::string label = sel
                ? std::string("[bold bright_white on_blue] \xe2\x96\xb6 ") + style_names[i] + " [/]"
                : std::string("  [dim]  ") + style_names[i] + "[/]";
            page.add(text_builder(label));
        }

        page.add(text_builder(""));
        page.add(rule_builder("Preview").rule_style(style{colors::bright_black}));

        // Show the selected status bar style
        switch (st_.sb_style) {
        case 0:
            page.add(status_bar_builder()
                .left("[bold] NORMAL [/]")
                .center("[bold]main.cpp[/] [dim](modified)[/]")
                .right("[dim]Ln 142, Col 28[/]")
                .style_override(style{colors::bright_white, color::from_rgb(30, 30, 80)}));
            break;
        case 1:
            page.add(status_bar_builder()
                .left("[bold] BUILD [/]")
                .center("[bold]Release x64[/]")
                .right("[bold green]0 errors, 2 warnings[/]")
                .style_override(style{colors::bright_white, color::from_rgb(20, 60, 20)}));
            break;
        case 2:
            page.add(status_bar_builder()
                .left("[bold] ERROR [/]")
                .center("ECONNREFUSED 127.0.0.1:5432")
                .right("[bold]Retry?[/]")
                .style_override(style{colors::bright_white, color::from_rgb(120, 20, 20)}));
            break;
        case 3:
            page.add(status_bar_builder()
                .left("tapiru v0.1.0")
                .right("Ready"));
            break;
        }

        page.key("sb-page");
        return page.flatten_into(s);
    }
private:
    const app_state& st_;
};

// ── Page 2: Rows & Spacer ───────────────────────────────────────────────

class rows_page_view {
public:
    explicit rows_page_view(const app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Rows & Spacer Demo[/]"));

        // Controls
        {
            char buf[80];
            std::snprintf(buf, sizeof(buf),
                "  [dim]Gap:[/] [bold cyan]%d[/] [dim](Left/Right to adjust)[/]", st_.row_gap);
            page.add(text_builder(buf));
        }

        const char* mode_names[] = {"Push Apart", "Centered", "Three-Section"};
        for (int i = 0; i < 3; ++i) {
            bool sel = (st_.spacer_mode == i);
            std::string label = sel
                ? std::string("[bold bright_white on_blue] \xe2\x96\xb6 ") + mode_names[i] + " [/]"
                : std::string("  [dim]  ") + mode_names[i] + "[/]";
            page.add(text_builder(label));
        }

        page.add(text_builder(""));
        page.add(rule_builder("Preview").rule_style(style{colors::bright_black}));

        // Preview panel
        {
            columns_builder preview;
            switch (st_.spacer_mode) {
            case 0:  // Push apart
                preview.add(text_builder("[bold green]LEFT[/]"));
                preview.add(spacer_builder(), 1);
                preview.add(text_builder("[bold red]RIGHT[/]"));
                break;
            case 1:  // Centered
                preview.add(spacer_builder(), 1);
                preview.add(text_builder("[bold bright_yellow]CENTERED CONTENT[/]"));
                preview.add(spacer_builder(), 1);
                break;
            case 2:  // Three-section
                preview.add(text_builder("[bold] MODE [/]"));
                preview.add(spacer_builder(), 1);
                preview.add(text_builder("[bold cyan]file.cpp[/]"));
                preview.add(spacer_builder(), 1);
                preview.add(text_builder("[dim]UTF-8 | C++[/]"));
                break;
            }
            preview.gap(0);
            preview.key("spacer-preview");

            panel_builder pb(std::move(preview));
            pb.border(border_style::rounded);
            pb.key("spacer-panel");
            page.add(std::move(pb));
        }

        // Rows with configurable gap
        page.add(text_builder(""));
        page.add(text_builder("  [bold]Rows with gap:[/]"));
        {
            rows_builder demo_rows;
            demo_rows.add(text_builder("  [cyan]Row A[/]"));
            demo_rows.add(text_builder("  [yellow]Row B[/]"));
            demo_rows.add(text_builder("  [green]Row C[/]"));
            demo_rows.add(text_builder("  [red]Row D[/]"));
            demo_rows.gap(static_cast<uint32_t>(st_.row_gap));
            demo_rows.key("gap-rows");
            page.add(std::move(demo_rows));
        }

        page.key("rows-page");
        return page.flatten_into(s);
    }
private:
    const app_state& st_;
};

// ── Page 3: Table Visuals ───────────────────────────────────────────────

class table_page_view {
public:
    explicit table_page_view(const app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Table Visual Effects[/]"));
        page.add(text_builder("  [dim]Use Up/Down to select, Enter to apply[/]"));
        page.add(text_builder(""));

        const char* effect_names[] = {"No Effect", "Drop Shadow", "Green Glow", "Red Shadow (custom)"};
        for (int i = 0; i < 4; ++i) {
            bool sel = (st_.table_effect == i);
            std::string label = sel
                ? std::string("[bold bright_white on_blue] \xe2\x96\xb6 ") + effect_names[i] + " [/]"
                : std::string("  [dim]  ") + effect_names[i] + "[/]";
            page.add(text_builder(label));
        }

        page.add(text_builder(""));

        // Table with selected effect
        {
            table_builder tb;
            tb.add_column("Widget", {justify::left, 18, 25});
            tb.add_column("Status", {justify::center, 10, 15});
            tb.add_column("Type", {justify::right, 10, 15});
            tb.border(border_style::rounded);
            tb.header_style(style{colors::bright_cyan, {}, attr::bold});
            tb.key("demo-table");

            tb.add_row({"menu_builder",       "[green]Done[/]", "Native"});
            tb.add_row({"popup_builder",      "[green]Done[/]", "Composite"});
            tb.add_row({"status_bar_builder", "[green]Done[/]", "Composite"});
            tb.add_row({"rows_builder",       "[green]Done[/]", "Layout"});
            tb.add_row({"spacer_builder",     "[green]Done[/]", "Layout"});
            tb.add_row({"tqdm",               "[green]Done[/]", "Iterator"});

            switch (st_.table_effect) {
            case 1: tb.shadow(); break;
            case 2: tb.glow(color::from_rgb(0, 200, 0)); break;
            case 3: tb.shadow(3, 2, 2, color::from_rgb(180, 0, 0), 100); break;
            default: break;
            }

            page.add(std::move(tb));
        }

        page.key("table-page");
        return page.flatten_into(s);
    }
private:
    const app_state& st_;
};

// ── Page 4: Dashboard ───────────────────────────────────────────────────

class dashboard_page_view {
public:
    explicit dashboard_page_view(const app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        rows_builder page;
        page.gap(0);

        // Header
        page.add(text_builder("[bold bright_white on_blue]  Dashboard  [/]"));

        // Body: sidebar menu + content
        columns_builder body;

        // Sidebar
        {
            body.add(
                menu_builder()
                    .add_item("Overview")
                    .add_item("Tasks")
                    .add_item("Metrics")
                    .add_separator()
                    .add_item("Settings")
                    .cursor(&st_.dash_cursor)
                    .highlight_style(style{colors::bright_white, colors::blue, attr::bold})
                    .border(border_style::rounded)
                    .key("dash-menu")
            );
        }

        // Content area depends on dash_cursor
        {
            rows_builder content;
            content.gap(0);

            switch (st_.dash_cursor) {
            case 0: {  // Overview
                content.add(text_builder("[bold]System Overview[/]"));
                content.add(rule_builder());

                auto t1 = std::make_shared<progress_task>("CPU",  100);
                auto t2 = std::make_shared<progress_task>("Mem",  100);
                auto t3 = std::make_shared<progress_task>("Disk", 100);
                int cpu_val = 50 + static_cast<int>(30.0 * std::sin(st_.tick * 0.15));
                t1->advance(cpu_val);
                t2->advance(62);
                t3->advance(34);
                content.add(progress_builder()
                    .add_task(t1).add_task(t2).add_task(t3)
                    .bar_width(20).key("sys-progress"));
                break;
            }
            case 1: {  // Tasks
                content.add(text_builder("[bold]Task List[/]"));
                content.add(rule_builder());
                table_builder tb;
                tb.add_column("Task", {justify::left, 18, 25});
                tb.add_column("Done", {justify::center, 6, 8});
                tb.border(border_style::none);
                tb.header_style(style{colors::bright_cyan, {}, attr::bold});
                tb.add_row({"Widget expansion", "[green]\xe2\x9c\x93[/]"});
                tb.add_row({"tqdm iterator",    "[green]\xe2\x9c\x93[/]"});
                tb.add_row({"Interactive demo", "[yellow]...[/]"});
                tb.add_row({"Documentation",    "[dim]\xe2\x97\x8b[/]"});
                tb.key("task-table");
                content.add(std::move(tb));
                break;
            }
            case 2: {  // Metrics
                content.add(text_builder("[bold]Metrics[/]"));
                content.add(rule_builder());
                std::vector<float> data;
                for (int i = 0; i < 40; ++i)
                    data.push_back(static_cast<float>(
                        std::sin((i + st_.tick) * 0.2) * 30 + 50));
                content.add(line_chart_builder(data, 30, 4)
                    .style_override(style{colors::bright_green})
                    .key("metric-chart"));
                break;
            }
            default: {  // Settings
                content.add(text_builder("[bold]Settings[/]"));
                content.add(rule_builder());
                content.add(text_builder("  Theme:    [cyan]Dark[/]"));
                content.add(text_builder("  Font:     [cyan]14px[/]"));
                content.add(text_builder("  Tab:      [cyan]4 spaces[/]"));
                break;
            }
            }

            content.key("dash-content");

            panel_builder cp(std::move(content));
            cp.border(border_style::rounded);
            cp.key("content-panel");
            body.add(std::move(cp), 1);
        }

        body.gap(1);
        body.key("dash-body");
        page.add(std::move(body));

        // Status bar at bottom
        char tick_buf[32];
        std::snprintf(tick_buf, sizeof(tick_buf), "Frame %d", st_.tick);
        page.add(status_bar_builder()
            .left("[bold] ONLINE [/]")
            .center(tick_buf)
            .right("[green]All systems nominal[/]")
            .style_override(style{colors::bright_white, color::from_rgb(30, 30, 60)}));

        page.key("dash-page");
        return page.flatten_into(s);
    }
private:
    const app_state& st_;
};

// ── Page 5: tqdm Demo ───────────────────────────────────────────────────

class tqdm_page_view {
public:
    explicit tqdm_page_view(const app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  tqdm Progress Iterator[/]"));
        page.add(text_builder(""));
        page.add(text_builder("  [bold]API Examples:[/]"));
        page.add(text_builder("  [dim]for (auto& x : tapiru::tqdm(vec))           { ... }[/]"));
        page.add(text_builder("  [dim]for (auto& x : tapiru::tqdm(vec, \"Load\"))   { ... }[/]"));
        page.add(text_builder("  [dim]for (int i : tapiru::trange(100))            { ... }[/]"));
        page.add(text_builder("  [dim]for (int i : tapiru::trange(0, 1000, 10))    { ... }[/]"));
        page.add(text_builder(""));
        page.add(text_builder("  [bold]Builder options:[/]"));
        page.add(text_builder("  [dim].desc() .bar_width() .unit() .total() .disable()[/]"));
        page.add(text_builder("  [dim].leave() .miniters() .mininterval() .colour()[/]"));
        page.add(text_builder(""));

        if (st_.tqdm_running) {
            char buf[80];
            double pct = static_cast<double>(st_.tqdm_progress) / st_.tqdm_total * 100.0;
            std::snprintf(buf, sizeof(buf),
                "  [bold yellow]Running:[/] %d/%d [bold](%.0f%%)[/]",
                st_.tqdm_progress, st_.tqdm_total, pct);
            page.add(text_builder(buf));

            // Visual progress bar
            int bar_w = 40;
            int filled = static_cast<int>(pct / 100.0 * bar_w);
            std::string bar = "  [bright_green]";
            for (int i = 0; i < filled; ++i) bar += "\xe2\x96\x88";
            bar += "[/][dim]";
            for (int i = filled; i < bar_w; ++i) bar += "\xe2\x96\x91";
            bar += "[/]";
            page.add(text_builder(bar));
        } else {
            page.add(text_builder("  [bold]Press Enter to run tqdm demo[/]"));
            page.add(text_builder("  [dim](Runs trange(50) with 60ms sleep per iteration)[/]"));
        }

        page.key("tqdm-page");
        return page.flatten_into(s);
    }
private:
    const app_state& st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Composite view — header + page + footer
// ═══════════════════════════════════════════════════════════════════════

class app_view {
public:
    explicit app_view(const app_state& st) : st_(st) {}  // copies the state

    node_id flatten_into(detail::scene& s) const {
        rows_builder root;
        root.gap(0);

        // Header
        root.add(text_builder(""));
        root.add(text_builder("  [bold bright_yellow]tapiru[/] [dim]Interactive New Features Demo[/]"));
        root.add(rule_builder().rule_style(style{colors::bright_black}));
        root.add(build_tab_bar(st_));
        root.add(rule_builder().rule_style(style{colors::bright_black}));

        // Page content
        switch (st_.page) {
        case 0: { auto v = menu_page_view(st_);      root.add(std::move(v)); break; }
        case 1: { auto v = statusbar_page_view(st_);  root.add(std::move(v)); break; }
        case 2: { auto v = rows_page_view(st_);       root.add(std::move(v)); break; }
        case 3: { auto v = table_page_view(st_);      root.add(std::move(v)); break; }
        case 4: { auto v = dashboard_page_view(st_);   root.add(std::move(v)); break; }
        case 5: { auto v = tqdm_page_view(st_);        root.add(std::move(v)); break; }
        }

        // Footer
        root.add(text_builder(""));
        root.add(rule_builder().rule_style(style{colors::bright_black}));
        root.add(build_footer(st_));

        root.key("app-root");
        return root.flatten_into(s);
    }
private:
    app_state st_;  // COPY — render thread gets its own snapshot
};

// ═══════════════════════════════════════════════════════════════════════
//  Input handling
// ═══════════════════════════════════════════════════════════════════════

static int max_cursor(const app_state& st) {
    switch (st.page) {
    case 0: return 7;   // menu items
    case 1: return 3;   // status bar styles
    case 2: return 2;   // spacer modes
    case 3: return 3;   // table effects
    case 4: return 4;   // dashboard sidebar (including separator)
    case 5: return 0;
    default: return 0;
    }
}

static void handle_input(app_state& st, input_ev ev) {
    // Popup dismissal takes priority
    if (st.show_popup && (ev == input_ev::enter || ev == input_ev::quit || ev == input_ev::space)) {
        st.show_popup = false;
        if (ev == input_ev::quit) return;  // don't also quit
        return;
    }

    switch (ev) {
    case input_ev::tab:
        st.page = (st.page + 1) % NUM_PAGES;
        st.cursor = 0;
        st.show_popup = false;
        break;
    case input_ev::shift_tab:
        st.page = (st.page + NUM_PAGES - 1) % NUM_PAGES;
        st.cursor = 0;
        st.show_popup = false;
        break;
    case input_ev::up:
        if (st.page == 0) { st.menu_cursor = menu_cursor_prev(st.menu_cursor); }
        else if (st.page == 4) { if (st.dash_cursor > 0) --st.dash_cursor; }
        else { if (st.cursor > 0) --st.cursor; }
        break;
    case input_ev::down:
        if (st.page == 0) { st.menu_cursor = menu_cursor_next(st.menu_cursor); }
        else if (st.page == 4) { if (st.dash_cursor < 4) ++st.dash_cursor; }
        else { if (st.cursor < max_cursor(st)) ++st.cursor; }
        break;
    case input_ev::enter:
    case input_ev::space:
        if (st.page == 0 && !is_menu_separator(st.menu_cursor)) {
            st.show_popup = true;
            st.popup_item = st.menu_cursor;
        } else if (st.page == 1) {
            st.sb_style = st.cursor;
        } else if (st.page == 2) {
            st.spacer_mode = st.cursor;
        } else if (st.page == 3) {
            st.table_effect = st.cursor;
        } else if (st.page == 5 && !st.tqdm_running) {
            st.tqdm_running = true;
            st.tqdm_progress = 0;
        }
        break;
    case input_ev::left:
        if (st.page == 1) { st.sb_style = st.cursor; }
        else if (st.page == 2) { if (st.row_gap > 0) --st.row_gap; }
        break;
    case input_ev::right:
        if (st.page == 1) { st.sb_style = st.cursor; }
        else if (st.page == 2) { if (st.row_gap < 3) ++st.row_gap; }
        break;
    case input_ev::mouse_click:
        st.last_click_x = g_mouse_x;
        st.last_click_y = g_mouse_y;
        st.click_count++;
        // Click on tab bar area (row ~3) to switch pages
        if (g_mouse_y >= 2 && g_mouse_y <= 4) {
            int col = 2;
            for (int i = 0; i < NUM_PAGES; ++i) {
                int name_len = static_cast<int>(std::strlen(page_names[i])) + 2;
                if (g_mouse_x >= col && g_mouse_x < col + name_len) {
                    st.page = i;
                    st.cursor = 0;
                    st.show_popup = false;
                    break;
                }
                col += name_len + 1;
            }
        }
        break;
    case input_ev::quit:
        st.quit = true;
        break;
    default:
        break;
    }

    // Sync cursor with page-specific state
    if (st.page == 1) st.sb_style = st.cursor;
    if (st.page == 2) st.spacer_mode = st.cursor;
    if (st.page == 3) st.table_effect = st.cursor;
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    if (!terminal::is_tty()) {
        std::fprintf(stderr, "Interactive demo requires a terminal (TTY).\n");
        return 1;
    }

    console con;

    // Alternate screen + hide cursor
    con.write("\x1b[?1049h");
    con.write("\x1b[?25l");

    enable_raw_input();

    app_state st;
    auto sz = terminal::get_size();
    uint32_t width = sz.width > 0 ? sz.width : 80;

    auto last_tqdm_tick = std::chrono::steady_clock::now();

    // Initial render
    con.write("\x1b[H");
    con.print_widget(app_view(st), width);
    con.write("\x1b[J");

    while (!st.quit) {
        auto ev = poll_input(50);
        bool changed = (ev != input_ev::none);
        handle_input(st, ev);

        st.tick++;

        // tqdm simulation
        if (st.tqdm_running) {
            auto now = std::chrono::steady_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tqdm_tick).count();
            if (dt >= 60) {
                st.tqdm_progress++;
                last_tqdm_tick = now;
                changed = true;
                if (st.tqdm_progress >= st.tqdm_total) {
                    st.tqdm_running = false;
                    st.tqdm_progress = st.tqdm_total;
                }
            }
        }

        // Animated pages always re-render
        bool anim = (st.page == 4 && st.tick % 10 == 0);
        if (changed || anim) {
            con.write("\x1b[H");  // cursor home
            con.print_widget(app_view(st), width);
            con.write("\x1b[J");  // clear to end of screen
        }
    }

    restore_input();

    // Restore screen + show cursor
    con.write("\x1b[?25h");
    con.write("\x1b[?1049l");

    // Run actual tqdm after exiting TUI to show real terminal output
    std::printf("Thanks for trying the interactive demo!\n\n");
    std::printf("Running real tqdm demo...\n");

    for ([[maybe_unused]] int i : tapiru::trange(50).desc("Processing").bar_width(35).unit("it")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }

    std::printf("\n");

    std::vector<std::string> files = {"main.cpp", "utils.h", "render.cpp", "scene.h", "input.cpp"};
    for ([[maybe_unused]] auto& f : tapiru::tqdm(files, "Compiling").unit("files").bar_width(25)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::printf("\nDone!\n");
    return 0;
}
