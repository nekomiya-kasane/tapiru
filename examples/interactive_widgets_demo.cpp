/**
 * @file interactive_widgets_demo.cpp
 * @brief Interactive TUI demo showcasing all 17 new widgets + transition animations.
 *
 * Multi-page demo with keyboard navigation:
 *   Page 0: Tabs & Accordion   — tab switching, collapsible sections
 *   Page 1: Menu & Dropdown    — multi-level menu, dropdown select
 *   Page 2: Tree & Scroll      — tree view navigation, scroll view
 *   Page 3: Form Controls      — toggles, search input, textarea
 *   Page 4: Notifications      — toast, log viewer, breadcrumb
 *   Page 5: Layout & Keys      — grid layout, keybinding display
 *   Page 6: Animations         — typewriter, counter, marquee, cross-fade
 *
 * Controls:
 *   Tab / Shift+Tab  — switch pages
 *   Up / Down        — navigate within page
 *   Enter / Space    — toggle / activate
 *   Left / Right     — adjust values
 *   1-4              — trigger toast notifications
 *   q / Escape       — quit
 *
 * Build:
 *   cmake --build build --target tapiru_interactive_widgets_demo --config Release
 * Run:
 *   .\build\bin\Release\tapiru_interactive_widgets_demo.exe
 */

#ifdef _WIN32
#    include <Windows.h>
#else
#    include <sys/select.h>
#    include <termios.h>
#    include <unistd.h>
#endif

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

// ── Core ────────────────────────────────────────────────────────────────
#include "tapiru/core/console.h"
#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"
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

// ═══════════════════════════════════════════════════════════════════════
//  Input abstraction (same pattern as interactive_demo.cpp)
// ═══════════════════════════════════════════════════════════════════════

enum class input_ev : uint8_t {
    none,
    up,
    down,
    left,
    right,
    enter,
    tab,
    shift_tab,
    quit,
    space,
    key_1,
    key_2,
    key_3,
    key_4
};

#ifdef _WIN32

static HANDLE g_stdin = INVALID_HANDLE_VALUE;
static DWORD g_old_mode = 0;

static void enable_raw_input() {
    g_stdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(g_stdin, &g_old_mode);
    SetConsoleMode(g_stdin, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT);
}

static void restore_input() {
    if (g_stdin != INVALID_HANDLE_VALUE) {
        SetConsoleMode(g_stdin, g_old_mode);
    }
}

static input_ev poll_input(int timeout_ms) {
    if (WaitForSingleObject(g_stdin, static_cast<DWORD>(timeout_ms)) != WAIT_OBJECT_0) {
        return input_ev::none;
    }
    INPUT_RECORD rec;
    DWORD count = 0;
    if (!ReadConsoleInputW(g_stdin, &rec, 1, &count) || count == 0) {
        return input_ev::none;
    }
    if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
        auto vk = rec.Event.KeyEvent.wVirtualKeyCode;
        auto ch = rec.Event.KeyEvent.uChar.UnicodeChar;
        bool shift = (rec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) != 0;
        if (vk == VK_UP) {
            return input_ev::up;
        }
        if (vk == VK_DOWN) {
            return input_ev::down;
        }
        if (vk == VK_LEFT) {
            return input_ev::left;
        }
        if (vk == VK_RIGHT) {
            return input_ev::right;
        }
        if (vk == VK_RETURN) {
            return input_ev::enter;
        }
        if (vk == VK_TAB) {
            return shift ? input_ev::shift_tab : input_ev::tab;
        }
        if (vk == VK_ESCAPE) {
            return input_ev::quit;
        }
        if (ch == L'q' || ch == L'Q') {
            return input_ev::quit;
        }
        if (ch == L' ') {
            return input_ev::space;
        }
        if (ch == L'1') {
            return input_ev::key_1;
        }
        if (ch == L'2') {
            return input_ev::key_2;
        }
        if (ch == L'3') {
            return input_ev::key_3;
        }
        if (ch == L'4') {
            return input_ev::key_4;
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
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) {
        return input_ev::none;
    }
    char buf[16];
    int n = static_cast<int>(read(STDIN_FILENO, buf, sizeof(buf)));
    if (n <= 0) {
        return input_ev::none;
    }
    if (n == 1) {
        if (buf[0] == 'q' || buf[0] == 'Q' || buf[0] == 27) {
            return input_ev::quit;
        }
        if (buf[0] == '\n' || buf[0] == '\r') {
            return input_ev::enter;
        }
        if (buf[0] == ' ') {
            return input_ev::space;
        }
        if (buf[0] == '\t') {
            return input_ev::tab;
        }
        if (buf[0] == '1') {
            return input_ev::key_1;
        }
        if (buf[0] == '2') {
            return input_ev::key_2;
        }
        if (buf[0] == '3') {
            return input_ev::key_3;
        }
        if (buf[0] == '4') {
            return input_ev::key_4;
        }
    } else if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
        if (buf[2] == 'A') {
            return input_ev::up;
        }
        if (buf[2] == 'B') {
            return input_ev::down;
        }
        if (buf[2] == 'C') {
            return input_ev::right;
        }
        if (buf[2] == 'D') {
            return input_ev::left;
        }
        if (buf[2] == 'Z') {
            return input_ev::shift_tab;
        }
    }
    return input_ev::none;
}

#endif

// ═══════════════════════════════════════════════════════════════════════
//  Application State
// ═══════════════════════════════════════════════════════════════════════

static constexpr int NUM_PAGES = 7;
static const char *page_names[NUM_PAGES] = {"Tabs", "Menu", "Tree", "Form", "Notify", "Layout", "Anim"};

struct app_state {
    bool quit = false;
    int page = 0;
    int tick = 0;

    // Page 0: Tabs & Accordion
    int active_tab = 0;
    std::vector<bool> accordion_expanded = {true, false, false};
    int accordion_cursor = 0;

    // Page 1: Menu & Dropdown
    int menu_cursor = 0;
    int dropdown_selected = 1;
    bool dropdown_open = false;

    // Page 2: Tree & Scroll
    int tree_cursor = 0;
    std::unordered_set<std::string> tree_expanded = {"src", "include"};
    int scroll_y = 0;

    // Page 3: Form Controls
    bool toggle_dark_mode = true;
    bool toggle_notifications = false;
    bool toggle_autosave = true;
    std::string search_query = "widget";
    int search_match = 2;
    int search_total = 5;
    std::string textarea_content = "#include <tapiru/core/console.h>\n"
                                   "\n"
                                   "int main() {\n"
                                   "    tapiru::console con;\n"
                                   "    con.print(\"[bold]Hello![/]\");\n"
                                   "    return 0;\n"
                                   "}";
    int textarea_cursor_row = 3;
    int textarea_cursor_col = 4;
    int textarea_scroll_row = 0;

    // Page 4: Notifications
    toast_state toast;
    std::vector<log_entry> logs = {
        {log_level::info, "12:00:01", "Application started"},
        {log_level::info, "12:00:02", "Loading configuration..."},
        {log_level::warn, "12:00:03", "Config key 'theme' not found"},
        {log_level::info, "12:00:04", "Server listening on :8080"},
        {log_level::error, "12:00:05", "Failed to connect to DB"},
        {log_level::info, "12:00:06", "Retrying connection..."},
        {log_level::info, "12:00:07", "Database connected"},
        {log_level::debug, "12:00:08", "Query executed in 12ms"},
    };
    int log_scroll = 0;
    int breadcrumb_active = 3;

    // Page 6: Animations
    typewriter tw{"Hello, tapiru world!", duration_ms(2000)};
    counter_animation counter{0.0, 1000.0, duration_ms(3000), easing::ease_out};
    marquee mrq{"  tapiru — A modern C++ terminal UI library with rich widgets, animations, and adaptive rendering  ",
                40, duration_ms(150)};
    bool anim_started = false;
    time_point anim_start;
};

// ═══════════════════════════════════════════════════════════════════════
//  Shared UI helpers
// ═══════════════════════════════════════════════════════════════════════

static auto build_tab_bar(const app_state &st) {
    std::string tabs = "  ";
    for (int i = 0; i < NUM_PAGES; ++i) {
        if (i == st.page) {
            tabs += std::string("[bold bright_white on_blue] ") + page_names[i] + " [/]";
        } else {
            tabs += std::string("[dim] ") + page_names[i] + " [/]";
        }
        if (i < NUM_PAGES - 1) {
            tabs += " ";
        }
    }
    return text_builder(tabs);
}

static auto build_footer(const app_state &st) {
    (void)st;
    return text_builder("  [dim]Tab[/] page  [dim]\xe2\x86\x91\xe2\x86\x93[/] navigate  "
                        "[dim]Enter[/] select  [dim]\xe2\x86\x90\xe2\x86\x92[/] adjust  "
                        "[dim]1-4[/] toast  [dim]q[/] quit");
}

// ═══════════════════════════════════════════════════════════════════════
//  Page 0: Tabs & Accordion
// ═══════════════════════════════════════════════════════════════════════

class tabs_page_view {
  public:
    explicit tabs_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Tabs & Accordion[/]"));
        page.add(text_builder("  [dim]Left/Right switch tabs, Up/Down + Enter toggle accordion[/]"));
        page.add(text_builder(""));

        // Tab widget
        {
            auto tb = tab_builder();
            tb.add_tab("Overview", text_builder("  [bold]Overview[/]\n"
                                                "  This demo showcases 17 new widgets added to tapiru.\n"
                                                "  Use [bold cyan]Left/Right[/] to switch between tabs."));
            tb.add_tab("Features", text_builder("  [bold]Features[/]\n"
                                                "  \xe2\x80\xa2 sized_box, center, scroll_view\n"
                                                "  \xe2\x80\xa2 multi-level menu, menu_bar, tab\n"
                                                "  \xe2\x80\xa2 accordion, tree_view, dropdown\n"
                                                "  \xe2\x80\xa2 toggle, toast, textarea\n"
                                                "  \xe2\x80\xa2 log_viewer, search_input, breadcrumb\n"
                                                "  \xe2\x80\xa2 keybinding, grid"));
            tb.add_tab("About", text_builder("  [bold]About[/]\n"
                                             "  tapiru is a modern C++ terminal UI library.\n"
                                             "  Zero dependencies. Header-friendly. Beautiful output."));
            tb.active(&st_.active_tab);
            tb.active_tab_style(style{colors::bright_white, colors::blue, attr::bold});
            tb.tab_style(style{colors::bright_black});
            tb.content_border(border_style::rounded);
            tb.key("demo-tabs");
            page.add(std::move(tb));
        }

        page.add(text_builder(""));

        // Accordion
        {
            auto acc = accordion_builder();
            acc.add_section("\xe2\x96\xb6 Getting Started",
                            text_builder("    Install via CMake:\n"
                                         "    [cyan]add_subdirectory(tapiru)[/]\n"
                                         "    [cyan]target_link_libraries(myapp PRIVATE tapiru)[/]"));
            acc.add_section("\xe2\x96\xb6 Configuration",
                            text_builder("    [bold]console_config[/] options:\n"
                                         "    \xe2\x80\xa2 sink — custom output function\n"
                                         "    \xe2\x80\xa2 depth — color depth override\n"
                                         "    \xe2\x80\xa2 force_color / no_color"));
            acc.add_section("\xe2\x96\xb6 Advanced", text_builder("    [bold]Live engine[/] for real-time TUI:\n"
                                                                  "    [cyan]tapiru::live lv(con);[/]\n"
                                                                  "    [cyan]lv.set(my_widget);[/]"));
            acc.expanded(&st_.accordion_expanded);
            acc.header_style(style{colors::bright_white, {}, attr::bold});
            acc.active_header_style(style{colors::bright_cyan, {}, attr::bold});
            acc.border(border_style::rounded);
            acc.key("demo-accordion");

            // Show cursor indicator
            rows_builder acc_section;
            acc_section.gap(0);
            for (int i = 0; i < 3; ++i) {
                std::string indicator = (i == st_.accordion_cursor) ? "[bold bright_yellow]\xe2\x96\xb8[/] " : "  ";
                acc_section.add(text_builder(
                    indicator + "Section " + std::to_string(i + 1) +
                    (st_.accordion_expanded[static_cast<size_t>(i)] ? " [green](open)[/]" : " [dim](closed)[/]")));
            }
            page.add(std::move(acc_section));
            page.add(std::move(acc));
        }

        page.key("tabs-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 1: Menu & Dropdown
// ═══════════════════════════════════════════════════════════════════════

class menu_page_view {
  public:
    explicit menu_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Menu & Dropdown[/]"));
        page.add(text_builder("  [dim]Up/Down navigate menu, Left/Right cycle dropdown[/]"));
        page.add(text_builder(""));

        columns_builder body;

        // Left: multi-level menu
        {
            auto mb = menu_builder();
            mb.add(menu_item_builder("New File").shortcut("Ctrl+N"));
            mb.add(menu_item_builder("Open Recent")
                       .add(menu_item_builder("project.cpp"))
                       .add(menu_item_builder("main.h"))
                       .add(menu_item_builder("CMakeLists.txt")));
            mb.add_separator();
            mb.add(menu_item_builder("Save").shortcut("Ctrl+S"));
            mb.add(menu_item_builder("Save As...").shortcut("Ctrl+Shift+S"));
            mb.add_separator();
            mb.add(menu_item_builder("Exit").shortcut("Ctrl+Q"));
            mb.cursor(&st_.menu_cursor);
            mb.border(border_style::rounded);
            mb.highlight_style(style{colors::bright_white, colors::blue, attr::bold});
            mb.shortcut_style(style{colors::bright_black});
            mb.shadow();
            mb.key("file-menu");
            body.add(std::move(mb));
        }

        // Right: dropdown + info
        {
            rows_builder right;
            right.gap(1);

            right.add(text_builder("[bold]Theme Selector:[/]"));

            auto dd = dropdown_builder();
            dd.add_option("Dark Mode");
            dd.add_option("Light Mode");
            dd.add_option("Solarized");
            dd.add_option("Monokai");
            dd.add_option("Nord");
            dd.selected(&st_.dropdown_selected);
            dd.open(&st_.dropdown_open);
            dd.placeholder("Choose theme...");
            dd.highlight_style(style{colors::bright_white, colors::blue, attr::bold});
            dd.border(border_style::rounded);
            dd.width(20);
            dd.key("theme-dropdown");
            right.add(std::move(dd));

            right.add(text_builder(""));

            // Menu bar preview
            right.add(text_builder("[bold]Menu Bar Preview:[/]"));
            {
                menu_bar_state bar_state;
                auto bar = menu_bar_builder();
                bar.add_menu("File", {
                                         menu_item_builder("New").shortcut("Ctrl+N"),
                                         menu_item_builder("Open").shortcut("Ctrl+O"),
                                     });
                bar.add_menu("Edit", {
                                         menu_item_builder("Undo").shortcut("Ctrl+Z"),
                                         menu_item_builder("Redo").shortcut("Ctrl+Y"),
                                     });
                bar.add_menu("View", {
                                         menu_item_builder("Zoom In").shortcut("Ctrl++"),
                                         menu_item_builder("Zoom Out").shortcut("Ctrl+-"),
                                     });
                bar.state(&bar_state);
                bar.bar_style(style{colors::white, color::from_rgb(40, 40, 60)});
                bar.active_style(style{colors::bright_white, colors::blue, attr::bold});
                bar.key("preview-bar");
                right.add(std::move(bar));
            }

            right.key("menu-right");
            body.add(std::move(right), 1);
        }

        body.gap(2);
        body.key("menu-body");
        page.add(std::move(body));

        page.key("menu-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 2: Tree & Scroll
// ═══════════════════════════════════════════════════════════════════════

class tree_page_view {
  public:
    explicit tree_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Tree View & Scroll View[/]"));
        page.add(text_builder("  [dim]Up/Down navigate tree, Enter expand/collapse, Left/Right scroll[/]"));
        page.add(text_builder(""));

        columns_builder body;

        // Left: tree view
        {
            tree_node root;
            root.label = "project";
            root.children = {
                {"src",
                 {
                     {"main.cpp", {}},
                     {"console.cpp", {}},
                     {"canvas.cpp", {}},
                     {"live.cpp", {}},
                 }},
                {"include",
                 {
                     {"tapiru",
                      {
                          {"core",
                           {
                               {"console.h", {}},
                               {"style.h", {}},
                               {"canvas.h", {}},
                           }},
                          {"widgets",
                           {
                               {"builders.h", {}},
                               {"menu.h", {}},
                               {"tab.h", {}},
                           }},
                      }},
                 }},
                {"CMakeLists.txt", {}},
                {"README.md", {}},
            };

            auto tv = tree_view_builder();
            tv.root(std::move(root));
            tv.expanded_set(&st_.tree_expanded);
            tv.cursor(&st_.tree_cursor);
            tv.highlight_style(style{colors::bright_white, colors::blue, attr::bold});
            tv.guide_style(style{colors::bright_black});
            tv.key("file-tree");

            panel_builder tp(std::move(tv));
            tp.title("File Explorer");
            tp.border(border_style::rounded);
            tp.key("tree-panel");
            body.add(std::move(tp));
        }

        // Right: scroll view
        {
            rows_builder content;
            for (int i = 0; i < 30; ++i) {
                char buf[80];
                std::snprintf(buf, sizeof(buf), "  [dim]%3d[/] %s", i + 1,
                              (i % 5 == 0) ? "[bold cyan]// Section header[/]"
                                           : "    Lorem ipsum dolor sit amet, consectetur");
                content.add(text_builder(buf));
            }

            auto sv = scroll_view_builder(std::move(content));
            sv.viewport_width(40).viewport_height(12);
            sv.scroll_y(&st_.scroll_y);
            sv.show_scrollbar_v(true);
            sv.border(border_style::rounded);
            sv.key("code-scroll");

            rows_builder right;
            right.gap(0);
            right.add(text_builder("[bold]Scrollable Code View:[/]"));
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "  [dim]Scroll offset:[/] [bold cyan]%d[/]", st_.scroll_y);
                right.add(text_builder(buf));
            }
            right.add(std::move(sv));
            right.key("scroll-right");
            body.add(std::move(right), 1);
        }

        body.gap(2);
        body.key("tree-body");
        page.add(std::move(body));

        page.key("tree-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 3: Form Controls
// ═══════════════════════════════════════════════════════════════════════

class form_page_view {
  public:
    explicit form_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Form Controls[/]"));
        page.add(text_builder("  [dim]Up/Down select toggle, Enter to flip, Left/Right cycle search[/]"));
        page.add(text_builder(""));

        columns_builder body;

        // Left: toggles + search
        {
            rows_builder left;
            left.gap(1);

            left.add(text_builder("[bold]Toggle Switches:[/]"));

            auto t1 = toggle_builder();
            t1.value(&st_.toggle_dark_mode);
            t1.label("Dark Mode");
            t1.on_style(style{colors::bright_green, {}, attr::bold});
            t1.off_style(style{colors::bright_black});
            t1.key("toggle-dark");
            left.add(std::move(t1));

            auto t2 = toggle_builder();
            t2.value(&st_.toggle_notifications);
            t2.label("Notifications");
            t2.on_style(style{colors::bright_green, {}, attr::bold});
            t2.off_style(style{colors::bright_black});
            t2.key("toggle-notif");
            left.add(std::move(t2));

            auto t3 = toggle_builder();
            t3.value(&st_.toggle_autosave);
            t3.label("Auto-save");
            t3.on_style(style{colors::bright_green, {}, attr::bold});
            t3.off_style(style{colors::bright_black});
            t3.key("toggle-save");
            left.add(std::move(t3));

            left.add(text_builder(""));
            left.add(text_builder("[bold]Search Input:[/]"));

            auto si = search_input_builder();
            si.query(&st_.search_query);
            si.match_count(&st_.search_total);
            si.current_match(&st_.search_match);
            si.placeholder("Search...");
            si.width(25);
            si.border(border_style::rounded);
            si.key("search-box");
            left.add(std::move(si));

            left.key("form-left");
            body.add(std::move(left));
        }

        // Right: textarea
        {
            rows_builder right;
            right.gap(0);

            right.add(text_builder("[bold]Text Editor:[/]"));

            auto ta = textarea_builder();
            ta.text(&st_.textarea_content);
            ta.cursor_row(&st_.textarea_cursor_row);
            ta.cursor_col(&st_.textarea_cursor_col);
            ta.scroll_row(&st_.textarea_scroll_row);
            ta.width(40);
            ta.height(8);
            ta.show_line_numbers(true);
            ta.cursor_style(style{colors::bright_white, colors::blue});
            ta.line_number_style(style{colors::bright_black});
            ta.border(border_style::rounded);
            ta.key("code-editor");
            right.add(std::move(ta));

            right.key("form-right");
            body.add(std::move(right), 1);
        }

        body.gap(2);
        body.key("form-body");
        page.add(std::move(body));

        page.key("form-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 4: Notifications
// ═══════════════════════════════════════════════════════════════════════

class notify_page_view {
  public:
    explicit notify_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Notifications & Logs[/]"));
        page.add(text_builder(
            "  [dim]1=info 2=success 3=warning 4=error toast, Up/Down scroll log, Left/Right breadcrumb[/]"));
        page.add(text_builder(""));

        // Breadcrumb
        {
            int bc_active = st_.breadcrumb_active;
            auto bc = breadcrumb_builder();
            bc.add_item("Home");
            bc.add_item("Dashboard");
            bc.add_item("Settings");
            bc.add_item("Notifications");
            bc.active(&bc_active);
            bc.item_style(style{colors::bright_black});
            bc.active_style(style{colors::bright_cyan, {}, attr::bold});
            bc.separator_style(style{colors::bright_black});
            bc.key("nav-breadcrumb");
            page.add(std::move(bc));
        }

        page.add(text_builder(""));

        columns_builder body;

        // Left: log viewer
        {
            auto lv = log_viewer_builder();
            lv.entries(&st_.logs);
            lv.scroll(&st_.log_scroll);
            lv.height(8);
            lv.show_timestamp(true);
            lv.show_level(true);
            lv.border(border_style::rounded);
            lv.key("log-view");

            rows_builder left;
            left.gap(0);
            left.add(text_builder("[bold]Log Viewer:[/]"));
            left.add(std::move(lv));
            left.key("notify-left");
            body.add(std::move(left), 1);
        }

        // Right: toast preview
        {
            rows_builder right;
            right.gap(1);

            right.add(text_builder("[bold]Toast Notifications:[/]"));
            right.add(text_builder("  Press [bold]1[/] info"));
            right.add(text_builder("  Press [bold]2[/] success"));
            right.add(text_builder("  Press [bold]3[/] warning"));
            right.add(text_builder("  Press [bold]4[/] error"));

            if (st_.toast.visible()) {
                right.add(text_builder(""));
                auto tb = toast_builder();
                tb.state(&st_.toast);
                tb.border(border_style::rounded);
                tb.key("active-toast");
                right.add(std::move(tb));
            }

            right.key("notify-right");
            body.add(std::move(right));
        }

        body.gap(2);
        body.key("notify-body");
        page.add(std::move(body));

        page.key("notify-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 5: Layout & Keys
// ═══════════════════════════════════════════════════════════════════════

class layout_page_view {
  public:
    explicit layout_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Grid Layout & Keybindings[/]"));
        page.add(text_builder("  [dim]Static display of grid and keybinding widgets[/]"));
        page.add(text_builder(""));

        // Grid of status cards
        {
            auto g = grid_builder();
            g.columns(3);
            g.col_gap(1);
            g.row_gap(0);

            {
                panel_builder c1(text_builder("[bold]CPU[/]\n[bright_green]42%[/]"));
                c1.border(border_style::rounded);
                g.add(std::move(c1));
            }
            {
                panel_builder c2(text_builder("[bold]Memory[/]\n[bright_yellow]2.1 GB[/]"));
                c2.border(border_style::rounded);
                g.add(std::move(c2));
            }
            {
                panel_builder c3(text_builder("[bold]Disk[/]\n[bright_cyan]67%[/]"));
                c3.border(border_style::rounded);
                g.add(std::move(c3));
            }
            {
                panel_builder c4(text_builder("[bold]Network[/]\n[bright_green]12 MB/s[/]"));
                c4.border(border_style::rounded);
                g.add(std::move(c4));
            }
            {
                panel_builder c5(text_builder("[bold]Threads[/]\n[bright_white]24[/]"));
                c5.border(border_style::rounded);
                g.add(std::move(c5));
            }
            {
                panel_builder c6(text_builder("[bold]Uptime[/]\n[dim]3d 14h[/]"));
                c6.border(border_style::rounded);
                g.add(std::move(c6));
            }

            g.key("status-grid");
            page.add(std::move(g));
        }

        page.add(text_builder(""));

        // Keybinding reference
        {
            page.add(text_builder("[bold]Keyboard Shortcuts:[/]"));
            auto kb = keybinding_builder();
            kb.add("Tab", "Next page");
            kb.add("Shift+Tab", "Previous page");
            kb.add("\xe2\x86\x91\xe2\x86\x93", "Navigate");
            kb.add("Enter", "Select / Toggle");
            kb.add("\xe2\x86\x90\xe2\x86\x92", "Adjust value");
            kb.add("1-4", "Trigger toast");
            kb.add("q / Esc", "Quit");
            kb.key_style(style{colors::bright_cyan, color::from_rgb(30, 40, 60), attr::bold});
            kb.desc_style(style{colors::white});
            kb.key("shortcut-ref");
            page.add(std::move(kb));
        }

        page.key("layout-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 6: Animations
// ═══════════════════════════════════════════════════════════════════════

class anim_page_view {
  public:
    explicit anim_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Transition Animations[/]"));
        page.add(text_builder("  [dim]Press Enter to restart animations[/]"));
        page.add(text_builder(""));

        auto now = std::chrono::steady_clock::now();

        // Typewriter
        {
            std::string tw_text = st_.tw.started() ? st_.tw.value(now) : "";
            bool done = st_.tw.started() && st_.tw.finished(now);
            std::string label = done ? "  [bold]Typewriter:[/] [bold green]" + tw_text + "[/] [dim]\xe2\x9c\x93[/]"
                                     : "  [bold]Typewriter:[/] [bold cyan]" + tw_text + "[/][dim]\xe2\x96\x8c[/]";
            page.add(text_builder(label));
        }

        page.add(text_builder(""));

        // Counter
        {
            int val = st_.counter.started() ? st_.counter.int_value(now) : 0;
            bool done = st_.counter.started() && st_.counter.finished(now);
            char buf[80];
            std::snprintf(buf, sizeof(buf), "  [bold]Counter:[/] [bold bright_yellow]%d[/]%s", val,
                          done ? " [dim]\xe2\x9c\x93[/]" : "");
            page.add(text_builder(buf));
        }

        page.add(text_builder(""));

        // Marquee
        {
            std::string mrq_text = st_.mrq.started() ? st_.mrq.value(now) : "";
            page.add(text_builder("  [bold]Marquee:[/]"));

            panel_builder mrq_panel(text_builder("[bold bright_green]" + mrq_text + "[/]"));
            mrq_panel.border(border_style::rounded);
            mrq_panel.border_style_override(style{colors::bright_black});
            mrq_panel.key("marquee-box");
            page.add(std::move(mrq_panel));
        }

        page.add(text_builder(""));

        // Animation status
        if (!st_.anim_started) {
            page.add(text_builder("  [bold bright_yellow]Press Enter to start animations[/]"));
        } else {
            bool all_done = st_.tw.finished(now) && st_.counter.finished(now);
            if (all_done) {
                page.add(text_builder("  [bold green]All animations complete![/] [dim]Press Enter to restart[/]"));
            } else {
                page.add(text_builder("  [dim]Animations running...[/]"));
            }
        }

        page.key("anim-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Composite view — header + page + footer
// ═══════════════════════════════════════════════════════════════════════

class app_view {
  public:
    explicit app_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder root;
        root.gap(0);

        // Header
        root.add(text_builder(""));
        root.add(text_builder("  [bold bright_yellow]tapiru[/] [dim]Interactive Widget Expansion Demo[/]"));
        root.add(rule_builder().rule_style(style{colors::bright_black}));
        root.add(build_tab_bar(st_));
        root.add(rule_builder().rule_style(style{colors::bright_black}));

        // Page content
        switch (st_.page) {
        case 0: {
            auto v = tabs_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 1: {
            auto v = menu_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 2: {
            auto v = tree_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 3: {
            auto v = form_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 4: {
            auto v = notify_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 5: {
            auto v = layout_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 6: {
            auto v = anim_page_view(st_);
            root.add(std::move(v));
            break;
        }
        }

        // Footer
        root.add(text_builder(""));
        root.add(rule_builder().rule_style(style{colors::bright_black}));
        root.add(build_footer(st_));

        root.key("app-root");
        return root.flatten_into(s);
    }

  private:
    app_state st_; // COPY — render gets its own snapshot
};

// ═══════════════════════════════════════════════════════════════════════
//  Input handling
// ═══════════════════════════════════════════════════════════════════════

// Tree node labels in DFS order for cursor mapping
static const char *tree_labels[] = {"project", "src",        "main.cpp", "console.cpp", "canvas.cpp",     "live.cpp",
                                    "include", "tapiru",     "core",     "console.h",   "style.h",        "canvas.h",
                                    "widgets", "builders.h", "menu.h",   "tab.h",       "CMakeLists.txt", "README.md"};
static constexpr int TREE_NODE_COUNT = 18;

static void handle_input(app_state &st, input_ev ev) {
    // Toast tick (always)
    st.toast.tick(std::chrono::milliseconds(50));

    // Global toast triggers
    if (ev == input_ev::key_1) {
        st.toast.show("Operation completed successfully.", toast_level::info);
        return;
    }
    if (ev == input_ev::key_2) {
        st.toast.show("File saved!", toast_level::success);
        return;
    }
    if (ev == input_ev::key_3) {
        st.toast.show("Disk space running low.", toast_level::warning);
        return;
    }
    if (ev == input_ev::key_4) {
        st.toast.show("Connection failed!", toast_level::error);
        return;
    }

    switch (ev) {
    case input_ev::tab:
        st.page = (st.page + 1) % NUM_PAGES;
        break;
    case input_ev::shift_tab:
        st.page = (st.page + NUM_PAGES - 1) % NUM_PAGES;
        break;
    case input_ev::quit:
        st.quit = true;
        break;

    case input_ev::up:
        switch (st.page) {
        case 0:
            if (st.accordion_cursor > 0) {
                --st.accordion_cursor;
            }
            break;
        case 1:
            if (st.menu_cursor > 0) {
                --st.menu_cursor;
            }
            break;
        case 2:
            if (st.tree_cursor > 0) {
                --st.tree_cursor;
            }
            break;
        case 4:
            if (st.log_scroll > 0) {
                --st.log_scroll;
            }
            break;
        default:
            break;
        }
        break;

    case input_ev::down:
        switch (st.page) {
        case 0:
            if (st.accordion_cursor < 2) {
                ++st.accordion_cursor;
            }
            break;
        case 1:
            if (st.menu_cursor < 7) {
                ++st.menu_cursor;
            }
            break;
        case 2:
            if (st.tree_cursor < TREE_NODE_COUNT - 1) {
                ++st.tree_cursor;
            }
            break;
        case 4:
            if (st.log_scroll < static_cast<int>(st.logs.size()) - 1) {
                ++st.log_scroll;
            }
            break;
        default:
            break;
        }
        break;

    case input_ev::left:
        switch (st.page) {
        case 0:
            if (st.active_tab > 0) {
                --st.active_tab;
            }
            break;
        case 1:
            if (st.dropdown_selected > 0) {
                --st.dropdown_selected;
            }
            break;
        case 2:
            if (st.scroll_y > 0) {
                --st.scroll_y;
            }
            break;
        case 3:
            if (st.search_match > 0) {
                --st.search_match;
            }
            break;
        case 4:
            if (st.breadcrumb_active > 0) {
                --st.breadcrumb_active;
            }
            break;
        default:
            break;
        }
        break;

    case input_ev::right:
        switch (st.page) {
        case 0:
            if (st.active_tab < 2) {
                ++st.active_tab;
            }
            break;
        case 1:
            if (st.dropdown_selected < 4) {
                ++st.dropdown_selected;
            }
            break;
        case 2:
            if (st.scroll_y < 25) {
                ++st.scroll_y;
            }
            break;
        case 3:
            if (st.search_match < st.search_total - 1) {
                ++st.search_match;
            }
            break;
        case 4:
            if (st.breadcrumb_active < 3) {
                ++st.breadcrumb_active;
            }
            break;
        default:
            break;
        }
        break;

    case input_ev::enter:
    case input_ev::space:
        switch (st.page) {
        case 0: {
            // Toggle accordion section
            auto idx = static_cast<size_t>(st.accordion_cursor);
            if (idx < st.accordion_expanded.size()) {
                st.accordion_expanded[idx] = !st.accordion_expanded[idx];
            }
            break;
        }
        case 1:
            st.dropdown_open = !st.dropdown_open;
            break;
        case 2: {
            // Toggle tree node expansion
            if (st.tree_cursor >= 0 && st.tree_cursor < TREE_NODE_COUNT) {
                std::string label = tree_labels[st.tree_cursor];
                if (st.tree_expanded.count(label)) {
                    st.tree_expanded.erase(label);
                } else {
                    st.tree_expanded.insert(label);
                }
            }
            break;
        }
        case 3: {
            // Cycle through toggles (simplified: toggle dark mode)
            st.toggle_dark_mode = !st.toggle_dark_mode;
            break;
        }
        case 6: {
            // Start/restart animations
            auto now = std::chrono::steady_clock::now();
            st.tw = typewriter("Hello, tapiru world!", duration_ms(2000));
            st.counter = counter_animation(0.0, 1000.0, duration_ms(3000), easing::ease_out);
            st.mrq = marquee("  tapiru \xe2\x80\x94 A modern C++ terminal UI library with rich widgets, animations, "
                             "and adaptive rendering  ",
                             40, duration_ms(150));
            st.tw.start(now);
            st.counter.start(now);
            st.mrq.start(now);
            st.anim_started = true;
            st.anim_start = now;
            break;
        }
        default:
            break;
        }
        break;

    default:
        break;
    }
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

    // Initial render
    con.write("\x1b[H");
    con.print_widget(app_view(st), width);
    con.write("\x1b[J");

    while (!st.quit) {
        auto ev = poll_input(50);
        bool changed = (ev != input_ev::none);
        handle_input(st, ev);

        st.tick++;

        // Toast auto-tick even without input
        if (!changed) {
            st.toast.tick(std::chrono::milliseconds(50));
        }

        // Animated pages always re-render
        bool anim = (st.page == 6 && st.anim_started);
        bool toast_visible = st.toast.visible();

        if (changed || anim || toast_visible) {
            // Re-detect terminal size for responsive layout
            auto new_sz = terminal::get_size();
            if (new_sz.width > 0) {
                width = new_sz.width;
            }

            con.write("\x1b[H");
            con.print_widget(app_view(st), width);
            con.write("\x1b[J");
        }
    }

    restore_input();

    // Restore screen + show cursor
    con.write("\x1b[?25h");
    con.write("\x1b[?1049l");

    std::printf("Thanks for trying the interactive widgets demo!\n");
    return 0;
}
