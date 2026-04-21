/**
 * @file comprehensive_demo.cpp
 * @brief Comprehensive interactive TUI demo showcasing all Stage 1-7 features.
 *
 * Multi-page demo with keyboard navigation:
 *   Page 0: Canvas       — animated braille drawing, shape selection
 *   Page 1: Gauge        — adjustable gauges, direction toggle
 *   Page 2: Paragraph    — dynamic wrap width, justify toggle
 *   Page 3: Decorators   — toggle border/bold/italic/color/center pipes
 *   Page 4: Colors       — color downgrade visualization, depth toggle
 *   Page 5: Widgets      — checkbox, slider, text input, radio group
 *   Page 6: Dashboard    — combined canvas + gauge + paragraph + status
 *
 * Controls:
 *   Tab / Shift+Tab  — switch pages
 *   Up / Down        — navigate within page
 *   Enter / Space    — toggle / activate
 *   Left / Right     — adjust values
 *   q / Escape       — quit
 *
 * Build:
 *   cmake --build build --target tapiru_comprehensive_demo --config Debug
 * Run:
 *   .\build\bin\Debug\tapiru_comprehensive_demo.exe
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "tapiru/core/console.h"
#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/canvas_widget.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/gauge.h"
#include "tapiru/widgets/interactive.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/paragraph.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/status_bar.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
//  Input abstraction
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
    key_4,
    key_5,
    key_6,
    key_7
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
        if (ch == L'5') {
            return input_ev::key_5;
        }
        if (ch == L'6') {
            return input_ev::key_6;
        }
        if (ch == L'7') {
            return input_ev::key_7;
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
        if (buf[0] >= '1' && buf[0] <= '7') {
            return static_cast<input_ev>(static_cast<uint8_t>(input_ev::key_1) + (buf[0] - '1'));
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
static const char *page_names[NUM_PAGES] = {"Canvas", "Gauge",   "Paragraph", "Decorators",
                                            "Colors", "Widgets", "Dashboard"};

struct app_state {
    bool quit = false;
    int page = 0;
    int cursor = 0;
    int tick = 0;

    // Page 0: Canvas
    int canvas_shape = 0; // 0=circles, 1=sine, 2=star, 3=spiral
    bool canvas_animate = true;

    // Page 1: Gauge
    float gauge_values[4] = {0.73f, 0.45f, 0.12f, 0.88f};
    int gauge_selected = 0;
    bool gauge_vertical = false;

    // Page 2: Paragraph
    int para_width = 45;
    bool para_justify = false;
    int para_text_idx = 0;

    // Page 3: Decorators
    bool deco_border = true;
    bool deco_bold = false;
    bool deco_italic = false;
    bool deco_underline = false;
    bool deco_center = false;
    bool deco_color = false;
    bool deco_padding = false;

    // Page 4: Colors
    int color_depth = 3; // 0=none, 1=16, 2=256, 3=rgb
    int color_selected = 0;

    // Page 5: Widgets
    bool cb_values[3] = {true, false, true};
    float slider_val = 0.5f;
    int radio_sel = 0;
    int widget_focus = 0;

    // Page 6: Dashboard
    int dash_sidebar = 0;
};

// ═══════════════════════════════════════════════════════════════════════
//  Shared UI components
// ═══════════════════════════════════════════════════════════════════════

static auto build_tab_bar(const app_state &st) {
    std::string tabs = " ";
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
    return text_builder(" [dim]Tab[/] page  [dim]\xe2\x86\x91\xe2\x86\x93[/] navigate  "
                        "[dim]Enter/Space[/] toggle  [dim]\xe2\x86\x90\xe2\x86\x92[/] adjust  "
                        "[dim]1-7[/] jump  [dim]q[/] quit");
}

// ═══════════════════════════════════════════════════════════════════════
//  Page 0: Canvas — animated braille drawing
// ═══════════════════════════════════════════════════════════════════════

class canvas_page_view {
  public:
    explicit canvas_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Canvas Drawing API[/]  [dim](Stage 5)[/]"));
        page.add(text_builder(""));

        // Shape selector
        const char *shapes[] = {"Circles", "Sine Wave", "Star", "Spiral"};
        {
            std::string sel_line = "  Shape: ";
            for (int i = 0; i < 4; ++i) {
                if (i == st_.canvas_shape) {
                    sel_line += std::string("[bold bright_white on_blue] ") + shapes[i] + " [/] ";
                } else {
                    sel_line += std::string("[dim]") + shapes[i] + "[/] ";
                }
            }
            sel_line += st_.canvas_animate ? "  [green]ANIM ON[/]" : "  [dim]ANIM OFF[/]";
            page.add(text_builder(sel_line));
        }
        page.add(text_builder("  [dim]Up/Down: shape  Enter: toggle animation[/]"));
        page.add(text_builder(""));

        // Canvas
        int t = st_.canvas_animate ? st_.tick : 0;
        auto cvs = make_canvas(72, 32, [&](canvas_widget_builder &c) {
            c.draw_rect(0, 0, 71, 31, colors::bright_black);

            switch (st_.canvas_shape) {
            case 0: { // Concentric circles
                const color ring_colors[] = {colors::red,  colors::yellow, colors::green,
                                             colors::cyan, colors::blue,   colors::magenta};
                for (int r = 3; r <= 14; r += 2) {
                    int offset = st_.canvas_animate ? static_cast<int>(2.0 * std::sin(t * 0.1 + r * 0.3)) : 0;
                    c.draw_circle(36 + offset, 16, r, ring_colors[(r / 2) % 6]);
                }
                c.draw_text(2, 1, "Concentric Circles", style{colors::bright_yellow, {}, attr::bold});
                break;
            }
            case 1: { // Sine wave
                c.draw_line(0, 16, 71, 16, colors::bright_black);
                c.draw_line(0, 0, 0, 31, colors::bright_black);
                for (int x = 1; x < 72; ++x) {
                    int y = 16 + static_cast<int>(13.0 * std::sin((x + t * 2) * 0.12));
                    if (y >= 0 && y < 32) {
                        c.draw_point(x, y, colors::bright_green);
                    }
                }
                for (int x = 1; x < 72; ++x) {
                    int y = 16 + static_cast<int>(13.0 * std::cos((x + t * 2) * 0.12));
                    if (y >= 0 && y < 32) {
                        c.draw_point(x, y, colors::bright_cyan);
                    }
                }
                c.draw_text(2, 1, "sin(x)", style{colors::bright_green, {}, attr::bold});
                c.draw_text(2, 3, "cos(x)", style{colors::bright_cyan, {}, attr::bold});
                break;
            }
            case 2: { // Star
                int cx = 36, cy = 16;
                int outer = 14, inner = 6;
                double phase = st_.canvas_animate ? t * 0.05 : 0.0;
                for (int i = 0; i < 5; ++i) {
                    double a1 = phase + i * 2.0 * 3.14159 / 5.0 - 3.14159 / 2.0;
                    double a2 = phase + (i + 0.5) * 2.0 * 3.14159 / 5.0 - 3.14159 / 2.0;
                    double a3 = phase + (i + 1) * 2.0 * 3.14159 / 5.0 - 3.14159 / 2.0;
                    int x1 = cx + static_cast<int>(outer * std::cos(a1));
                    int y1 = cy + static_cast<int>(outer * std::sin(a1));
                    int x2 = cx + static_cast<int>(inner * std::cos(a2));
                    int y2 = cy + static_cast<int>(inner * std::sin(a2));
                    int x3 = cx + static_cast<int>(outer * std::cos(a3));
                    int y3 = cy + static_cast<int>(outer * std::sin(a3));
                    c.draw_line(x1, y1, x2, y2, colors::bright_yellow);
                    c.draw_line(x2, y2, x3, y3, colors::bright_yellow);
                }
                c.draw_text(2, 1, "Rotating Star", style{colors::bright_yellow, {}, attr::bold});
                break;
            }
            case 3: { // Spiral
                double phase = st_.canvas_animate ? t * 0.08 : 0.0;
                int px = 36, py = 16;
                for (int i = 0; i < 200; ++i) {
                    double a = phase + i * 0.1;
                    double r = 1.0 + i * 0.07;
                    int nx = 36 + static_cast<int>(r * std::cos(a));
                    int ny = 16 + static_cast<int>(r * std::sin(a));
                    if (nx >= 0 && nx < 72 && ny >= 0 && ny < 32) {
                        uint8_t g = static_cast<uint8_t>(100 + i * 0.7);
                        c.draw_point(nx, ny, color::from_rgb(0, g, static_cast<uint8_t>(255 - i * 0.7)));
                        if (i > 0) {
                            c.draw_line(px, py, nx, ny, color::from_rgb(0, g, static_cast<uint8_t>(255 - i * 0.7)));
                        }
                    }
                    px = nx;
                    py = ny;
                }
                c.draw_text(2, 1, "Spiral", style{colors::bright_cyan, {}, attr::bold});
                break;
            }
            }
        });
        page.add(std::move(cvs));

        page.key("canvas-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 1: Gauge — adjustable progress bars
// ═══════════════════════════════════════════════════════════════════════

class gauge_page_view {
  public:
    explicit gauge_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Gauge Widget[/]  [dim](Stage 6)[/]"));
        page.add(text_builder("  [dim]Up/Down: select  Left/Right: adjust  Enter: toggle direction[/]"));
        page.add(text_builder(""));

        const char *labels[] = {"CPU Usage", "Memory   ", "Disk I/O ", "Network  "};
        const style gauge_styles[] = {
            {colors::bright_green, {}, attr::bold},
            {colors::bright_cyan, {}, attr::bold},
            {colors::bright_yellow, {}, attr::bold},
            {colors::bright_magenta, {}, attr::bold},
        };
        const style dim_style{colors::bright_black, {}, attr::dim};

        if (!st_.gauge_vertical) {
            // Horizontal gauges
            for (int i = 0; i < 4; ++i) {
                columns_builder row;
                bool sel = (st_.gauge_selected == i);
                std::string label = sel ? std::string("[bold bright_white on_blue] \xe2\x96\xb6 ") + labels[i] + "[/]"
                                        : std::string("   ") + labels[i];
                row.add(text_builder(label));

                char pct[16];
                std::snprintf(pct, sizeof(pct), " %3d%% ", static_cast<int>(st_.gauge_values[i] * 100));
                row.add(make_gauge(st_.gauge_values[i], gauge_styles[i], dim_style), 1);
                row.add(text_builder(pct));
                row.gap(1);
                page.add(std::move(row));
            }
        } else {
            // Vertical gauges
            page.add(text_builder("  [bold]Vertical Mode[/]"));
            columns_builder cols;
            for (int i = 0; i < 4; ++i) {
                rows_builder col;
                bool sel = (st_.gauge_selected == i);
                std::string label = sel ? std::string("[bold bright_white on_blue]") + labels[i] + "[/]"
                                        : std::string("[dim]") + labels[i] + "[/]";
                col.add(make_gauge_direction(st_.gauge_values[i], gauge_direction::vertical));
                char pct[8];
                std::snprintf(pct, sizeof(pct), "%d%%", static_cast<int>(st_.gauge_values[i] * 100));
                col.add(text_builder(pct));
                col.add(text_builder(label));
                col.gap(0);
                cols.add(std::move(col), 1);
            }
            cols.gap(2);
            page.add(std::move(cols));
        }

        page.add(text_builder(""));

        // Gauge in a panel
        {
            rows_builder inner;
            for (int i = 0; i < 4; ++i) {
                inner.add(text_builder(std::string("[bold]") + labels[i] + "[/]"));
                inner.add(make_gauge(st_.gauge_values[i], gauge_styles[i], dim_style));
            }
            inner.gap(0);
            panel_builder pb(std::move(inner));
            pb.title("System Monitor");
            pb.border(border_style::rounded);
            pb.border_style_override(style{colors::bright_cyan});
            page.add(std::move(pb));
        }

        page.key("gauge-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 2: Paragraph — dynamic word wrap
// ═══════════════════════════════════════════════════════════════════════

static const char *g_para_texts[] = {
    "The quick brown fox jumps over the lazy dog. "
    "This sentence demonstrates automatic word wrapping in the tapiru "
    "paragraph widget. Long text is broken at word boundaries to fit "
    "within the available width, producing clean multi-line output.",

    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
    "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
    "reprehenderit in voluptate velit esse cillum dolore eu fugiat.",

    "tapiru is a modern C++23 terminal UI library featuring a rich widget "
    "system with layout engine, braille canvas drawing, progress gauges, "
    "word-wrapping paragraphs, color depth downgrade, and a composable "
    "element/decorator pipe API. It supports both interactive full-screen "
    "TUI applications and streaming terminal output.",
};

class paragraph_page_view {
  public:
    explicit paragraph_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Paragraph Widget[/]  [dim](Stage 6)[/]"));

        {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "  Width: [bold cyan]%d[/]  Justify: %s  Text: [bold]%d/3[/]",
                          st_.para_width, st_.para_justify ? "[green]ON[/]" : "[dim]OFF[/]", st_.para_text_idx + 1);
            page.add(text_builder(buf));
        }
        page.add(text_builder("  [dim]Left/Right: width  Enter: justify  Up/Down: text[/]"));
        page.add(text_builder(""));

        // Paragraph in a panel
        {
            auto para = st_.para_justify ? make_paragraph_justify(g_para_texts[st_.para_text_idx])
                                         : make_paragraph(g_para_texts[st_.para_text_idx]);
            panel_builder pb(std::move(para));
            pb.title(st_.para_justify ? "Justified" : "Word Wrap");
            pb.border(border_style::rounded);
            page.add(std::move(pb));
        }

        page.key("para-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 3: Decorators — toggle element pipe decorators
// ═══════════════════════════════════════════════════════════════════════

class decorator_page_view {
  public:
    explicit decorator_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Element | Decorator Pipes[/]  [dim](Stage 1)[/]"));
        page.add(text_builder("  [dim]Up/Down: select  Enter: toggle[/]"));
        page.add(text_builder(""));

        // Toggle list
        struct toggle_item {
            const char *name;
            bool on;
        };
        toggle_item items[] = {
            {"Border (rounded)", st_.deco_border}, {"Bold", st_.deco_bold},     {"Italic", st_.deco_italic},
            {"Underline", st_.deco_underline},     {"Center", st_.deco_center}, {"Fg Color (cyan)", st_.deco_color},
            {"Padding (1,2)", st_.deco_padding},
        };

        for (int i = 0; i < 7; ++i) {
            bool sel = (st_.cursor == i);
            std::string line;
            if (sel) {
                line = "[bold bright_white on_blue] \xe2\x96\xb6 [/] ";
            } else {
                line = "   ";
            }
            line += items[i].on ? "[green]\xe2\x9c\x93[/] " : "[red]\xe2\x9c\x97[/] ";
            line += items[i].name;
            page.add(text_builder(line));
        }

        page.add(text_builder(""));
        page.add(rule_builder("Preview").rule_style(style{colors::bright_black}));
        page.add(text_builder(""));

        // Build the element with selected decorators
        element e = element(text_builder("[bold]Hello, tapiru![/] Decorator pipes in action."));
        if (st_.deco_padding) {
            e = std::move(e) | padding(1, 2);
        }
        if (st_.deco_border) {
            e = std::move(e) | border(border_style::rounded);
        }
        if (st_.deco_bold) {
            e = std::move(e) | bold();
        }
        if (st_.deco_italic) {
            e = std::move(e) | italic();
        }
        if (st_.deco_underline) {
            e = std::move(e) | underline();
        }
        if (st_.deco_color) {
            e = std::move(e) | fg_color(colors::bright_cyan);
        }
        if (st_.deco_center) {
            e = std::move(e) | center();
        }
        page.add(std::move(e));

        // Show the pipe expression
        page.add(text_builder(""));
        {
            std::string expr = "  [dim]element(text)";
            if (st_.deco_padding) {
                expr += " | padding(1,2)";
            }
            if (st_.deco_border) {
                expr += " | border()";
            }
            if (st_.deco_bold) {
                expr += " | bold()";
            }
            if (st_.deco_italic) {
                expr += " | italic()";
            }
            if (st_.deco_underline) {
                expr += " | underline()";
            }
            if (st_.deco_color) {
                expr += " | fg_color(cyan)";
            }
            if (st_.deco_center) {
                expr += " | center()";
            }
            expr += "[/]";
            page.add(text_builder(expr));
        }

        page.key("deco-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 4: Colors — downgrade visualization
// ═══════════════════════════════════════════════════════════════════════

class color_page_view {
  public:
    explicit color_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Color Downgrade[/]  [dim](Stage 7)[/]"));

        const char *depth_names[] = {"None", "16-color", "256-color", "True Color (RGB)"};
        {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "  Target depth: [bold cyan]%s[/]  [dim](Left/Right to change)[/]",
                          depth_names[st_.color_depth]);
            page.add(text_builder(buf));
        }
        page.add(text_builder(""));

        // Color table
        struct color_entry {
            const char *name;
            uint8_t r, g, b;
        };
        color_entry entries[] = {
            {"Pure Red", 255, 0, 0},  {"Pure Green", 0, 255, 0}, {"Pure Blue", 0, 0, 255},     {"Orange", 255, 128, 0},
            {"Magenta", 255, 0, 255}, {"Cyan", 0, 255, 255},     {"Gray(128)", 128, 128, 128}, {"White", 255, 255, 255},
        };

        table_builder tb;
        tb.add_column("Color", {justify::left, 14, 18});
        tb.add_column("Original RGB", {justify::center, 14, 18});
        tb.add_column("Downgraded", {justify::center, 14, 18});
        tb.border(border_style::rounded);
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});

        for (auto &e : entries) {
            auto c = color::from_rgb(e.r, e.g, e.b);
            auto d = c.downgrade(static_cast<uint8_t>(st_.color_depth));

            char rgb_str[32], down_str[48];
            std::snprintf(rgb_str, sizeof(rgb_str), "(%u,%u,%u)", e.r, e.g, e.b);

            if (d.is_default()) {
                std::snprintf(down_str, sizeof(down_str), "[dim]stripped[/]");
            } else if (d.kind == color_kind::indexed_16) {
                std::snprintf(down_str, sizeof(down_str), "16-idx %u", d.r);
            } else if (d.kind == color_kind::indexed_256) {
                std::snprintf(down_str, sizeof(down_str), "256-idx %u", d.r);
            } else {
                std::snprintf(down_str, sizeof(down_str), "RGB(%u,%u,%u)", d.r, d.g, d.b);
            }

            tb.add_row({e.name, rgb_str, down_str});
        }

        page.add(std::move(tb));

        page.add(text_builder(""));
        page.add(text_builder("  [dim]attr::double_underline = 0x0100 (SGR 21) — new in Stage 7[/]"));

        page.key("color-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 5: Widgets — interactive widget builders
// ═══════════════════════════════════════════════════════════════════════

class widgets_page_view {
  public:
    explicit widgets_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        page.add(text_builder("[bold bright_cyan]  Interactive Widgets[/]  [dim](Stage 3)[/]"));
        page.add(text_builder("  [dim]Up/Down: focus  Left/Right: adjust  Enter: toggle[/]"));
        page.add(text_builder(""));

        // Checkboxes
        {
            std::string hdr = (st_.widget_focus == 0) ? "[bold bright_white on_blue] \xe2\x96\xb6 Checkboxes [/]"
                                                      : "  [bold]Checkboxes[/]";
            page.add(text_builder(hdr));

            const char *cb_labels[] = {"Enable notifications", "Dark mode", "Auto-save"};
            for (int i = 0; i < 3; ++i) {
                std::string line = "    ";
                line += st_.cb_values[i] ? "[green]\xe2\x9c\x93[/] " : "[dim]\xe2\x97\x8b[/] ";
                line += cb_labels[i];
                page.add(text_builder(line));
            }
        }

        page.add(text_builder(""));

        // Slider
        {
            std::string hdr =
                (st_.widget_focus == 1) ? "[bold bright_white on_blue] \xe2\x96\xb6 Slider [/]" : "  [bold]Slider[/]";
            page.add(text_builder(hdr));

            // Visual slider bar
            int bar_w = 30;
            int filled = static_cast<int>(st_.slider_val * bar_w);
            std::string bar = "    [bright_cyan]";
            for (int i = 0; i < bar_w; ++i) {
                if (i == filled) {
                    bar += "[bold bright_white]\xe2\x97\x8f[/][bright_cyan]";
                } else if (i < filled) {
                    bar += "\xe2\x94\x81";
                } else {
                    bar += "[dim]\xe2\x94\x81[/][bright_cyan]";
                }
            }
            char pct[16];
            std::snprintf(pct, sizeof(pct), " %d%%", static_cast<int>(st_.slider_val * 100));
            bar += "[/]";
            bar += pct;
            page.add(text_builder(bar));
        }

        page.add(text_builder(""));

        // Radio group
        {
            std::string hdr = (st_.widget_focus == 2) ? "[bold bright_white on_blue] \xe2\x96\xb6 Radio Group [/]"
                                                      : "  [bold]Radio Group[/]";
            page.add(text_builder(hdr));

            const char *options[] = {"Light theme", "Dark theme", "System default"};
            for (int i = 0; i < 3; ++i) {
                std::string line = "    ";
                line += (st_.radio_sel == i) ? "[bright_cyan]\xe2\x97\x89[/] " : "[dim]\xe2\x97\x8b[/] ";
                line += options[i];
                page.add(text_builder(line));
            }
        }

        page.add(text_builder(""));

        // Gauge as widget
        {
            std::string hdr =
                (st_.widget_focus == 3) ? "[bold bright_white on_blue] \xe2\x96\xb6 Gauge [/]" : "  [bold]Gauge[/]";
            page.add(text_builder(hdr));
            page.add(make_gauge(st_.slider_val, style{colors::bright_green, {}, attr::bold},
                                style{colors::bright_black, {}, attr::dim}));
        }

        page.key("widgets-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page 6: Dashboard — combined showcase
// ═══════════════════════════════════════════════════════════════════════

class dashboard_page_view {
  public:
    explicit dashboard_page_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder page;
        page.gap(0);

        // Header
        page.add(text_builder("[bold bright_white on_blue]  tapiru Feature Dashboard  [/]"));

        // Body: sidebar + content
        columns_builder body;

        // Sidebar menu
        body.add(menu_builder()
                     .add_item("Overview")
                     .add_item("Canvas")
                     .add_item("Metrics")
                     .add_separator()
                     .add_item("About")
                     .cursor(&st_.dash_sidebar)
                     .highlight_style(style{colors::bright_white, colors::blue, attr::bold})
                     .border(border_style::rounded)
                     .key("dash-menu"));

        // Content
        {
            rows_builder content;
            content.gap(0);

            switch (st_.dash_sidebar) {
            case 0: { // Overview — gauges
                content.add(text_builder("[bold]System Overview[/]"));
                content.add(rule_builder());
                content.add(text_builder("[bold]CPU[/]"));
                float cpu = 0.5f + 0.3f * static_cast<float>(std::sin(st_.tick * 0.1));
                content.add(make_gauge(cpu, style{colors::bright_green, {}, attr::bold},
                                       style{colors::bright_black, {}, attr::dim}));
                content.add(text_builder("[bold]Memory[/]"));
                content.add(make_gauge(0.62f));
                content.add(text_builder("[bold]Disk[/]"));
                content.add(make_gauge(0.34f));
                break;
            }
            case 1: { // Canvas
                content.add(text_builder("[bold]Live Canvas[/]"));
                content.add(rule_builder());
                auto cvs = make_canvas(40, 16, [&](canvas_widget_builder &c) {
                    c.draw_rect(0, 0, 39, 15, colors::bright_black);
                    for (int x = 0; x < 40; ++x) {
                        int y = 8 + static_cast<int>(6.0 * std::sin((x + st_.tick * 2) * 0.15));
                        if (y >= 0 && y < 16) {
                            c.draw_point(x, y, colors::bright_green);
                        }
                    }
                    c.draw_circle(20, 8, 6, colors::bright_cyan);
                });
                content.add(std::move(cvs));
                break;
            }
            case 2: { // Metrics
                content.add(text_builder("[bold]Performance Metrics[/]"));
                content.add(rule_builder());
                std::vector<float> data;
                for (int i = 0; i < 30; ++i) {
                    data.push_back(static_cast<float>(std::sin((i + st_.tick) * 0.2) * 30 + 50));
                }
                content.add(
                    line_chart_builder(data, 25, 4).style_override(style{colors::bright_green}).key("metric-chart"));
                break;
            }
            default: { // About
                content.add(text_builder("[bold]About tapiru[/]"));
                content.add(rule_builder());
                auto para = make_paragraph("tapiru is a modern C++23 terminal UI library. "
                                           "Features: canvas drawing, gauges, paragraphs, "
                                           "color downgrade, decorator pipes, and more.");
                content.add(std::move(para));
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

        // Status bar
        char tick_buf[32];
        std::snprintf(tick_buf, sizeof(tick_buf), "Frame %d", st_.tick);
        page.add(status_bar_builder()
                     .left("[bold] LIVE [/]")
                     .center(tick_buf)
                     .right("[green]All systems nominal[/]")
                     .style_override(style{colors::bright_white, color::from_rgb(30, 30, 60)}));

        page.key("dash-page");
        return page.flatten_into(s);
    }

  private:
    const app_state &st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Composite app view
// ═══════════════════════════════════════════════════════════════════════

class app_view {
  public:
    explicit app_view(const app_state &st) : st_(st) {}

    node_id flatten_into(detail::scene &s) const {
        rows_builder root;
        root.gap(0);

        // Header
        root.add(text_builder(""));
        root.add(text_builder("  [bold bright_yellow]tapiru[/] [dim]Comprehensive Interactive Demo — Stage 1-7[/]"));
        root.add(rule_builder().rule_style(style{colors::bright_black}));
        root.add(build_tab_bar(st_));
        root.add(rule_builder().rule_style(style{colors::bright_black}));

        // Page
        switch (st_.page) {
        case 0: {
            auto v = canvas_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 1: {
            auto v = gauge_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 2: {
            auto v = paragraph_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 3: {
            auto v = decorator_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 4: {
            auto v = color_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 5: {
            auto v = widgets_page_view(st_);
            root.add(std::move(v));
            break;
        }
        case 6: {
            auto v = dashboard_page_view(st_);
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
    app_state st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Input handling
// ═══════════════════════════════════════════════════════════════════════

static int max_cursor(const app_state &st) {
    switch (st.page) {
    case 0:
        return 3; // canvas shapes
    case 1:
        return 3; // gauge bars
    case 2:
        return 2; // paragraph texts
    case 3:
        return 6; // decorator toggles
    case 4:
        return 0;
    case 5:
        return 3; // widget groups
    case 6:
        return 4; // dashboard sidebar
    default:
        return 0;
    }
}

static void handle_input(app_state &st, input_ev ev) {
    // Page jump via number keys
    if (ev >= input_ev::key_1 && ev <= input_ev::key_7) {
        int target = static_cast<int>(ev) - static_cast<int>(input_ev::key_1);
        if (target < NUM_PAGES) {
            st.page = target;
            st.cursor = 0;
        }
        return;
    }

    switch (ev) {
    case input_ev::tab:
        st.page = (st.page + 1) % NUM_PAGES;
        st.cursor = 0;
        break;
    case input_ev::shift_tab:
        st.page = (st.page + NUM_PAGES - 1) % NUM_PAGES;
        st.cursor = 0;
        break;
    case input_ev::up:
        if (st.page == 6) {
            if (st.dash_sidebar > 0) {
                --st.dash_sidebar;
            }
        } else if (st.cursor > 0) {
            --st.cursor;
        }
        // Sync page-specific state
        if (st.page == 0) {
            st.canvas_shape = st.cursor;
        }
        if (st.page == 1) {
            st.gauge_selected = st.cursor;
        }
        if (st.page == 5) {
            st.widget_focus = st.cursor;
        }
        break;
    case input_ev::down:
        if (st.page == 6) {
            if (st.dash_sidebar < 4) {
                ++st.dash_sidebar;
            }
        } else if (st.cursor < max_cursor(st)) {
            ++st.cursor;
        }
        if (st.page == 0) {
            st.canvas_shape = st.cursor;
        }
        if (st.page == 1) {
            st.gauge_selected = st.cursor;
        }
        if (st.page == 5) {
            st.widget_focus = st.cursor;
        }
        break;
    case input_ev::left:
        if (st.page == 1) {
            st.gauge_values[st.gauge_selected] = std::max(0.0f, st.gauge_values[st.gauge_selected] - 0.05f);
        } else if (st.page == 2) {
            st.para_width = std::max(20, st.para_width - 5);
        } else if (st.page == 4) {
            st.color_depth = std::max(0, st.color_depth - 1);
        } else if (st.page == 5) {
            if (st.widget_focus == 1) {
                st.slider_val = std::max(0.0f, st.slider_val - 0.05f);
            } else if (st.widget_focus == 2 && st.radio_sel > 0) {
                --st.radio_sel;
            }
        }
        break;
    case input_ev::right:
        if (st.page == 1) {
            st.gauge_values[st.gauge_selected] = std::min(1.0f, st.gauge_values[st.gauge_selected] + 0.05f);
        } else if (st.page == 2) {
            st.para_width = std::min(80, st.para_width + 5);
        } else if (st.page == 4) {
            st.color_depth = std::min(3, st.color_depth + 1);
        } else if (st.page == 5) {
            if (st.widget_focus == 1) {
                st.slider_val = std::min(1.0f, st.slider_val + 0.05f);
            } else if (st.widget_focus == 2 && st.radio_sel < 2) {
                ++st.radio_sel;
            }
        }
        break;
    case input_ev::enter:
    case input_ev::space:
        if (st.page == 0) {
            st.canvas_animate = !st.canvas_animate;
        } else if (st.page == 1) {
            st.gauge_vertical = !st.gauge_vertical;
        } else if (st.page == 2) {
            st.para_justify = !st.para_justify;
        } else if (st.page == 3) {
            // Toggle the selected decorator
            switch (st.cursor) {
            case 0:
                st.deco_border = !st.deco_border;
                break;
            case 1:
                st.deco_bold = !st.deco_bold;
                break;
            case 2:
                st.deco_italic = !st.deco_italic;
                break;
            case 3:
                st.deco_underline = !st.deco_underline;
                break;
            case 4:
                st.deco_center = !st.deco_center;
                break;
            case 5:
                st.deco_color = !st.deco_color;
                break;
            case 6:
                st.deco_padding = !st.deco_padding;
                break;
            }
        } else if (st.page == 5) {
            if (st.widget_focus == 0) {
                // Cycle through checkboxes
                static int cb_idx = 0;
                st.cb_values[cb_idx] = !st.cb_values[cb_idx];
                cb_idx = (cb_idx + 1) % 3;
            } else if (st.widget_focus == 2) {
                st.radio_sel = (st.radio_sel + 1) % 3;
            }
        }
        break;
    case input_ev::quit:
        st.quit = true;
        break;
    default:
        break;
    }

    // Paragraph text cycling via up/down on page 2
    if (st.page == 2) {
        st.para_text_idx = st.cursor;
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    if (!terminal::is_tty()) {
        std::fprintf(stderr, "This demo requires a terminal (TTY).\n");
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
        auto ev = poll_input(33); // ~30 fps
        bool changed = (ev != input_ev::none);
        handle_input(st, ev);

        st.tick++;

        // Animated pages always re-render
        bool anim = (st.page == 0 && st.canvas_animate) || (st.page == 6);

        if (changed || anim) {
            con.write("\x1b[H");
            con.print_widget(app_view(st), width);
            con.write("\x1b[J");
        }
    }

    restore_input();

    // Restore screen + show cursor
    con.write("\x1b[?25h");
    con.write("\x1b[?1049l");

    std::printf("Thanks for trying the comprehensive interactive demo!\n");
    return 0;
}
