/**
 * @file tui_demo.cpp
 * @brief Comprehensive interactive TUI demo showcasing all tapiru features.
 *
 * Multi-page demo with keyboard navigation:
 *   Page 1: Dashboard — todo list + live sine chart + progress bars
 *   Page 2: Interactive Widgets — checkbox, radio, slider, text input
 *   Page 3: Charts & Image Art — braille line/bar/scatter + half-block image
 *   Page 4: Theme Gallery — dark/light/monokai preset comparison
 *   Page 5: Structured Logging — live scrolling log panel
 *
 * Controls:
 *   Tab / Shift+Tab  — switch pages
 *   Up / Down        — navigate within page
 *   Enter / Space    — toggle / activate
 *   Left / Right     — adjust slider
 *   q / Escape       — quit
 *
 * Build:
 *   cmake --build build --target tapiru_tui_demo --config Debug
 * Run:
 *   .\build\bin\Debug\tapiru_tui_demo.exe
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
#include "tapiru/core/live.h"
#include "tapiru/core/logging.h"
#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"
#include "tapiru/core/theme.h"
#include "tapiru/text/emoji.h"
#include "tapiru/text/markdown.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/chart.h"
#include "tapiru/widgets/image.h"
#include "tapiru/widgets/interactive.h"
#include "tapiru/widgets/progress.h"

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
//  Input abstraction
// ═══════════════════════════════════════════════════════════════════════

enum class input_event : uint8_t {
    none, up, down, left, right, enter, tab, shift_tab, quit
};

#ifdef _WIN32

static HANDLE g_stdin = INVALID_HANDLE_VALUE;
static DWORD  g_old_mode = 0;

static void enable_raw_input() {
    g_stdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(g_stdin, &g_old_mode);
    SetConsoleMode(g_stdin, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
}

static void restore_input() {
    if (g_stdin != INVALID_HANDLE_VALUE) SetConsoleMode(g_stdin, g_old_mode);
}

static input_event poll_input(int timeout_ms) {
    if (WaitForSingleObject(g_stdin, static_cast<DWORD>(timeout_ms)) != WAIT_OBJECT_0)
        return input_event::none;
    INPUT_RECORD rec;
    DWORD count = 0;
    if (!ReadConsoleInputW(g_stdin, &rec, 1, &count) || count == 0)
        return input_event::none;
    if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
        auto vk = rec.Event.KeyEvent.wVirtualKeyCode;
        auto ch = rec.Event.KeyEvent.uChar.UnicodeChar;
        bool shift = (rec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) != 0;
        if (vk == VK_UP)     return input_event::up;
        if (vk == VK_DOWN)   return input_event::down;
        if (vk == VK_LEFT)   return input_event::left;
        if (vk == VK_RIGHT)  return input_event::right;
        if (vk == VK_RETURN) return input_event::enter;
        if (vk == VK_TAB)    return shift ? input_event::shift_tab : input_event::tab;
        if (vk == VK_ESCAPE) return input_event::quit;
        if (ch == L'q' || ch == L'Q') return input_event::quit;
        if (ch == L' ')      return input_event::enter;
    }
    return input_event::none;
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

static input_event poll_input(int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
        return input_event::none;
    char buf[16];
    int n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return input_event::none;
    if (n == 1) {
        if (buf[0] == 'q' || buf[0] == 'Q' || buf[0] == 27) return input_event::quit;
        if (buf[0] == '\n' || buf[0] == '\r' || buf[0] == ' ') return input_event::enter;
        if (buf[0] == '\t') return input_event::tab;
    } else if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
        if (buf[2] == 'A') return input_event::up;
        if (buf[2] == 'B') return input_event::down;
        if (buf[2] == 'C') return input_event::right;
        if (buf[2] == 'D') return input_event::left;
        if (buf[2] == 'Z') return input_event::shift_tab;
    }
    return input_event::none;
}

#endif

// ═══════════════════════════════════════════════════════════════════════
//  Application State
// ═══════════════════════════════════════════════════════════════════════

static constexpr int NUM_PAGES = 5;

static const char* page_names[NUM_PAGES] = {
    "Dashboard", "Widgets", "Charts", "Themes", "Logging"
};

struct todo_item {
    std::string text;
    bool done = false;
};

struct app_state {
    bool quit = false;
    int  page = 0;       // 0..4
    int  cursor = 0;     // per-page cursor
    int  tick = 0;        // frame counter for animation

    // Page 0: Dashboard
    std::vector<todo_item> todos = {
        {"Canvas swap optimization",   true},
        {"Live engine diff",           true},
        {"Gradient engine",            true},
        {"Border join algorithm",      true},
        {"Event routing",              true},
        {"Animation system",           true},
        {"Interactive widgets",        true},
        {"Braille charts",             true},
        {"Image character art",        true},
        {"Theme system",               false},
        {"Structured logging",         false},
        {"API documentation",          false},
    };

    // Page 1: Interactive widgets
    bool  cb_bold = true;
    bool  cb_italic = false;
    bool  cb_underline = false;
    int   radio_sel = 0;
    float slider_val = 0.65f;

    // Page 4: Logging
    log_panel_builder log_panel{50};
    int log_tick = 0;
};

// ═══════════════════════════════════════════════════════════════════════
//  Page renderers — each returns a markup string
// ═══════════════════════════════════════════════════════════════════════

static std::string render_tab_bar(const app_state& st) {
    std::string out = "  ";
    for (int i = 0; i < NUM_PAGES; ++i) {
        if (i == st.page) {
            out += "[bold bright_white on_blue] ";
            out += page_names[i];
            out += " [/]";
        } else {
            out += "[dim] ";
            out += page_names[i];
            out += " [/]";
        }
        if (i < NUM_PAGES - 1) out += " ";
    }
    return out;
}

// Dynamically set based on terminal height minus header/footer overhead
static int g_fixed_height = 30;

static int count_newlines(const std::string& s) {
    int n = 0;
    for (char c : s) if (c == '\n') ++n;
    return n;
}

static void pad_to_height(std::string& ui, int target = 0) {
    if (target <= 0) target = g_fixed_height;
    int lines = count_newlines(ui);
    while (lines < target) { ui += '\n'; ++lines; }
}

static std::string render_separator() {
    return "  [dim]\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
           "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80[/]";
}

// ── Page 0: Dashboard ──────────────────────────────────────────────────

static std::string render_dashboard(const app_state& st) {
    std::string ui;

    // Count done
    int done = 0;
    for (auto& t : st.todos) if (t.done) ++done;
    int total = static_cast<int>(st.todos.size());
    float pct = total > 0 ? static_cast<float>(done) / static_cast<float>(total) : 0.0f;

    // Progress summary
    char buf[64];
    std::snprintf(buf, sizeof(buf), "  [bold cyan]Progress:[/] %d/%d tasks  [bold green]%.0f%%[/]\n\n",
                  done, total, pct * 100.0f);
    ui += buf;

    // Todo list
    for (int i = 0; i < total; ++i) {
        bool sel = (i == st.cursor);
        const auto& item = st.todos[i];
        if (sel) {
            ui += "  [bold bright_white on_blue] ";
            ui += item.done ? "\xe2\x9c\x93" : " ";
            ui += " ";
            ui += item.text;
            // Pad
            int pad = 40 - static_cast<int>(item.text.size());
            if (pad > 0) ui += std::string(pad, ' ');
            ui += " [/]\n";
        } else {
            ui += "    ";
            if (item.done) {
                ui += "[green]\xe2\x9c\x93[/] [strike dim]";
                ui += item.text;
                ui += "[/]\n";
            } else {
                ui += "[dim]\xe2\x97\x8b[/] ";
                ui += item.text;
                ui += "\n";
            }
        }
    }

    // Sine wave chart (animated)
    ui += "\n  [bold yellow]CPU Usage (simulated)[/]\n";

    return ui;
}

// ── Page 1: Interactive Widgets ────────────────────────────────────────

static std::string render_widgets(const app_state& st) {
    std::string ui;

    ui += "  [bold bright_cyan]Style Options[/]\n\n";

    // Checkboxes
    auto cb = [&](const char* label, bool val, int idx) {
        bool sel = (st.cursor == idx);
        std::string prefix = sel ? "[bold bright_white on_blue] " : "    ";
        std::string suffix = sel ? " [/]" : "";
        ui += prefix;
        ui += val ? "[green]\xe2\x9c\x93[/] " : "[dim]\xe2\x97\x8b[/] ";
        ui += label;
        ui += suffix;
        ui += "\n";
    };
    cb("Bold",      st.cb_bold,      0);
    cb("Italic",    st.cb_italic,    1);
    cb("Underline", st.cb_underline, 2);

    // Preview
    ui += "\n  [dim]Preview:[/] ";
    std::string tags;
    if (st.cb_bold)      tags += "bold ";
    if (st.cb_italic)    tags += "italic ";
    if (st.cb_underline) tags += "underline ";
    if (tags.empty()) tags = "";
    if (!tags.empty()) {
        ui += "[" + tags + "]The quick brown fox[/]";
    } else {
        ui += "The quick brown fox";
    }
    ui += "\n\n";

    // Radio group
    ui += "  [bold bright_cyan]Border Style[/]\n\n";
    const char* radio_opts[] = {"Rounded", "Heavy", "Double", "ASCII"};
    for (int i = 0; i < 4; ++i) {
        bool sel = (st.cursor == 3 + i);
        std::string prefix = sel ? "[bold bright_white on_blue] " : "    ";
        std::string suffix = sel ? " [/]" : "";
        ui += prefix;
        ui += (st.radio_sel == i) ? "[bright_green]\xe2\x97\x89[/] " : "[dim]\xe2\x97\x8b[/] ";
        ui += radio_opts[i];
        ui += suffix;
        ui += "\n";
    }

    // Slider
    ui += "\n  [bold bright_cyan]Opacity[/]\n\n";
    {
        bool sel = (st.cursor == 7);
        std::string prefix = sel ? "[bold bright_white on_blue] " : "    ";
        std::string suffix = sel ? " [/]" : "";
        ui += prefix;

        int bar_w = 30;
        int filled = static_cast<int>(st.slider_val * bar_w);
        if (filled > bar_w) filled = bar_w;
        if (filled < 0) filled = 0;

        ui += "[bright_cyan]";
        for (int i = 0; i < filled; ++i) ui += "\xe2\x96\x88";
        ui += "[/][dim]";
        for (int i = filled; i < bar_w; ++i) ui += "\xe2\x96\x91";
        ui += "[/] ";

        char pct[8];
        std::snprintf(pct, sizeof(pct), "%3.0f%%", st.slider_val * 100.0f);
        ui += pct;
        ui += suffix;
        ui += "\n";
    }

    return ui;
}

// ── Page 2: Charts & Image ─────────────────────────────────────────────

static std::string render_charts_text(const app_state& st) {
    std::string ui;
    ui += "  [bold bright_yellow]Braille Line Chart[/] [dim](sine wave, animated)[/]\n\n";
    // Chart is rendered as a widget below
    ui += "\n\n\n\n\n\n\n\n";  // placeholder space for chart

    ui += "  [bold bright_yellow]Bar Chart[/] [dim](weekly data)[/]\n\n";
    ui += "\n\n\n\n\n\n\n\n\n";  // placeholder for bar chart

    ui += "  [bold bright_yellow]Half-Block Image[/] [dim](16x16 gradient)[/]\n";
    return ui;
}

// ── Page 3: Theme Gallery ──────────────────────────────────────────────

static std::string render_theme_gallery(const app_state& st) {
    std::string ui;

    auto show_theme = [&](const char* name, tapiru::theme th) {
        ui += "  [bold bright_white]";
        ui += name;
        ui += "[/]\n";

        const char* keys[] = {"text", "danger", "success", "warning", "info", "accent", "muted"};
        for (auto k : keys) {
            auto* s = th.lookup(k);
            if (!s) continue;
            char buf[80];
            std::snprintf(buf, sizeof(buf), "    [#%02X%02X%02X]%-10s[/]  fg=(%3u,%3u,%3u)",
                          s->fg.r, s->fg.g, s->fg.b, k, s->fg.r, s->fg.g, s->fg.b);
            ui += buf;
            if (has_attr(s->attrs, attr::bold)) ui += " [dim]+bold[/]";
            if (has_attr(s->attrs, attr::dim))  ui += " [dim]+dim[/]";
            if (has_attr(s->attrs, attr::underline)) ui += " [dim]+underline[/]";
            ui += "\n";
        }
        ui += "\n";
    };

    show_theme("Dark Theme",    tapiru::theme::dark());
    show_theme("Light Theme",   tapiru::theme::light());
    show_theme("Monokai Theme", tapiru::theme::monokai());

    return ui;
}

// ── Page 4: Structured Logging ─────────────────────────────────────────

static std::string render_logging_text(const app_state& st) {
    std::string ui;
    ui += "  [bold bright_magenta]Live Log Stream[/] [dim](auto-generated)[/]\n\n";
    // Log panel rendered as widget below
    return ui;
}

// ═══════════════════════════════════════════════════════════════════════
//  Composite view — builds the full widget tree
// ═══════════════════════════════════════════════════════════════════════

class app_view {
public:
    explicit app_view(app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        std::string ui;

        // Header
        ui += "\n  [bold bright_yellow]tapiru[/] [dim]Comprehensive TUI Demo[/]\n";
        ui += render_separator();
        ui += "\n";
        ui += render_tab_bar(st_);
        ui += "\n";
        ui += render_separator();
        ui += "\n\n";

        // Page content
        switch (st_.page) {
        case 0: ui += render_dashboard(st_); break;
        case 1: ui += render_widgets(st_); break;
        case 2: ui += render_charts_text(st_); break;
        case 3: ui += render_theme_gallery(st_); break;
        case 4: ui += render_logging_text(st_); break;
        }

        // Footer
        ui += "\n";
        ui += render_separator();
        ui += "\n";
        ui += "  [dim]Tab[/] next page  [dim]Shift+Tab[/] prev  "
              "[dim]Up/Down[/] navigate  [dim]Enter[/] toggle  "
              "[dim]Left/Right[/] adjust  [dim]q[/] quit\n";

        pad_to_height(ui, g_fixed_height);
        return text_builder(ui).overflow(overflow_mode::truncate).flatten_into(s);
    }

private:
    app_state& st_;
};

// Widget-based view for pages that need real widget rendering
class chart_page_view {
public:
    explicit chart_page_view(const app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        std::string ui;

        // Header
        ui += "\n  [bold bright_yellow]tapiru[/] [dim]Comprehensive TUI Demo[/]\n";
        ui += render_separator();
        ui += "\n";
        ui += render_tab_bar(st_);
        ui += "\n";
        ui += render_separator();
        ui += "\n\n";

        ui += "  [bold bright_yellow]Braille Line Chart[/] [dim](sine wave, animated)[/]\n";

        // We'll build the chart data inline
        std::vector<float> sine_data;
        for (int i = 0; i < 60; ++i) {
            float phase = static_cast<float>(st_.tick) * 0.1f;
            sine_data.push_back(static_cast<float>(
                std::sin((i + phase) * 0.15) * 40 + 50));
        }

        // Build as text with chart embedded
        ui += "\n";

        // Render chart to braille text
        braille_grid grid(30, 5);
        float mn = 0.0f, mx = 100.0f;
        for (size_t i = 0; i < sine_data.size(); ++i) {
            uint32_t dx = static_cast<uint32_t>(
                static_cast<float>(i) / static_cast<float>(sine_data.size()) *
                static_cast<float>(grid.dot_width()));
            float norm = (sine_data[i] - mn) / (mx - mn);
            if (norm < 0.0f) norm = 0.0f;
            if (norm > 1.0f) norm = 1.0f;
            uint32_t dy = static_cast<uint32_t>(
                (1.0f - norm) * static_cast<float>(grid.dot_height() - 1));
            grid.set(dx, dy);
        }
        std::string chart_str = grid.render();
        // Indent each line
        size_t pos = 0;
        while (pos < chart_str.size()) {
            size_t nl = chart_str.find('\n', pos);
            if (nl == std::string::npos) nl = chart_str.size();
            ui += "    [bright_cyan]";
            ui += chart_str.substr(pos, nl - pos);
            ui += "[/]\n";
            pos = nl + 1;
        }

        ui += "\n  [bold bright_yellow]Bar Chart[/] [dim](weekly data)[/]\n\n";

        // Bar chart using block characters
        float bar_data[] = {3, 7, 2, 9, 5, 8, 1, 6};
        const char* bar_labels[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Avg"};
        float bar_max = 9.0f;
        int bar_h = 6;
        for (int row = bar_h; row >= 1; --row) {
            ui += "    ";
            for (int b = 0; b < 8; ++b) {
                float threshold = bar_max * static_cast<float>(row) / static_cast<float>(bar_h);
                if (bar_data[b] >= threshold) {
                    ui += "[bright_green]\xe2\x96\x88\xe2\x96\x88[/] ";
                } else {
                    ui += "   ";
                }
            }
            ui += "\n";
        }
        ui += "    ";
        for (int b = 0; b < 8; ++b) {
            ui += "[dim]";
            ui += bar_labels[b];
            // Pad to 3 chars
            for (size_t j = std::strlen(bar_labels[b]); j < 3; ++j) ui += " ";
            ui += "[/]";
        }
        ui += "\n";

        ui += "\n  [bold bright_yellow]Half-Block Image[/] [dim](16x16 gradient)[/]\n\n";

        // We can't embed the image widget in markup, so show a note
        // The image will be shown via print_widget separately
        ui += "    [dim](rendered below via half-block characters)[/]\n";

        // Footer
        ui += "\n";
        ui += render_separator();
        ui += "\n";
        ui += "  [dim]Tab[/] next page  [dim]Shift+Tab[/] prev  "
              "[dim]Up/Down[/] navigate  [dim]Enter[/] toggle  "
              "[dim]Left/Right[/] adjust  [dim]q[/] quit\n";

        pad_to_height(ui, g_fixed_height);
        return text_builder(ui).overflow(overflow_mode::truncate).flatten_into(s);
    }

private:
    const app_state& st_;
};

class logging_page_view {
public:
    explicit logging_page_view(app_state& st) : st_(st) {}

    node_id flatten_into(detail::scene& s) const {
        std::string ui;

        // Header
        ui += "\n  [bold bright_yellow]tapiru[/] [dim]Comprehensive TUI Demo[/]\n";
        ui += render_separator();
        ui += "\n";
        ui += render_tab_bar(st_);
        ui += "\n";
        ui += render_separator();
        ui += "\n\n";

        ui += "  [bold bright_magenta]Live Log Stream[/] [dim](auto-generated every 500ms)[/]\n\n";

        // Render actual log entries with auto-scroll
        auto& lines = st_.log_panel.lines();
        // Calculate how many lines we can show (total height minus header/footer overhead)
        int avail = g_fixed_height - 12;  // ~12 lines for header + footer + padding
        if (avail < 3) avail = 3;
        size_t start = 0;
        size_t count = lines.size();
        if (count > static_cast<size_t>(avail)) {
            start = count - static_cast<size_t>(avail);
            count = static_cast<size_t>(avail);
        }
        for (size_t i = 0; i < count; ++i) {
            ui += "    ";
            ui += lines[start + i];
            ui += "\n";
        }
        if (lines.empty()) {
            ui += "    [dim](no logs yet)[/]\n";
        }

        // Footer
        ui += "\n";
        ui += render_separator();
        ui += "\n";
        ui += "  [dim]Tab[/] next page  [dim]Shift+Tab[/] prev  "
              "[dim]Up/Down[/] navigate  [dim]Enter[/] toggle  "
              "[dim]Left/Right[/] adjust  [dim]q[/] quit\n";

        pad_to_height(ui, g_fixed_height);
        return text_builder(ui).overflow(overflow_mode::truncate).flatten_into(s);
    }

private:
    app_state& st_;
};

// ═══════════════════════════════════════════════════════════════════════
//  Log generation
// ═══════════════════════════════════════════════════════════════════════

static void generate_log(app_state& st) {
    static const char* modules[] = {"http", "db", "auth", "cache", "worker"};
    static const char* messages[] = {
        "Request handled", "Query executed", "Token validated",
        "Cache miss", "Job completed", "Connection opened",
        "Timeout warning", "Rate limit hit", "Session expired",
        "Index rebuilt", "Backup started", "Health check OK"
    };
    static const log_level levels[] = {
        log_level::trace, log_level::debug, log_level::info,
        log_level::info, log_level::info, log_level::warn,
        log_level::warn, log_level::error
    };

    int idx = st.log_tick;
    log_record rec;
    rec.level = levels[idx % 8];
    rec.message = messages[idx % 12];
    rec.module = modules[idx % 5];

    // Add some structured fields
    if (idx % 3 == 0) {
        char dur[16];
        std::snprintf(dur, sizeof(dur), "%dms", (idx * 37 + 13) % 500);
        rec.fields["duration"] = dur;
    }
    if (idx % 4 == 0) {
        rec.fields["status"] = std::to_string(200 + (idx % 5) * 100);
    }

    st.log_panel.push(rec);
    st.log_tick++;
}

// ═══════════════════════════════════════════════════════════════════════
//  Input handling
// ═══════════════════════════════════════════════════════════════════════

static int max_cursor_for_page(const app_state& st) {
    switch (st.page) {
    case 0: return static_cast<int>(st.todos.size()) - 1;
    case 1: return 7;  // 3 checkboxes + 4 radio + 1 slider
    case 2: return 0;
    case 3: return 0;
    case 4: return 0;
    default: return 0;
    }
}

static void handle_input(app_state& st, input_event ev) {
    switch (ev) {
    case input_event::tab:
        st.page = (st.page + 1) % NUM_PAGES;
        st.cursor = 0;
        break;
    case input_event::shift_tab:
        st.page = (st.page + NUM_PAGES - 1) % NUM_PAGES;
        st.cursor = 0;
        break;
    case input_event::up:
        if (st.cursor > 0) --st.cursor;
        break;
    case input_event::down:
        if (st.cursor < max_cursor_for_page(st)) ++st.cursor;
        break;
    case input_event::enter:
        if (st.page == 0 && st.cursor < static_cast<int>(st.todos.size())) {
            st.todos[st.cursor].done = !st.todos[st.cursor].done;
        } else if (st.page == 1) {
            if (st.cursor == 0) st.cb_bold = !st.cb_bold;
            else if (st.cursor == 1) st.cb_italic = !st.cb_italic;
            else if (st.cursor == 2) st.cb_underline = !st.cb_underline;
            else if (st.cursor >= 3 && st.cursor <= 6) st.radio_sel = st.cursor - 3;
        }
        break;
    case input_event::left:
        if (st.page == 1 && st.cursor == 7) {
            st.slider_val -= 0.05f;
            if (st.slider_val < 0.0f) st.slider_val = 0.0f;
        }
        break;
    case input_event::right:
        if (st.page == 1 && st.cursor == 7) {
            st.slider_val += 0.05f;
            if (st.slider_val > 1.0f) st.slider_val = 1.0f;
        }
        break;
    case input_event::quit:
        st.quit = true;
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
        std::fprintf(stderr, "TUI demo requires an interactive terminal.\n");
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
    uint32_t height = sz.height > 0 ? sz.height : 30;

    // Set fixed height to fit within terminal (leave 1 line margin)
    g_fixed_height = static_cast<int>(height) - 1;
    if (g_fixed_height < 10) g_fixed_height = 10;

    // Pre-populate some log entries
    for (int i = 0; i < 8; ++i) generate_log(st);

    auto last_log = std::chrono::steady_clock::now();

    {
        live lv(con, 12, width);

        // Initial render
        lv.set(app_view(st));

        while (!st.quit) {
            auto ev = poll_input(50);

            bool changed = (ev != input_event::none);
            handle_input(st, ev);

            // Animate
            st.tick++;

            // Generate logs periodically
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_log).count() > 500) {
                generate_log(st);
                last_log = now;
                changed = true;
            }

            // Re-render on change, or every ~1s for animated chart page
            bool anim_tick = (st.page == 2 && st.tick % 20 == 0);
            if (changed || anim_tick) {
                if (st.page == 2) {
                    lv.set(chart_page_view(st));
                } else if (st.page == 4) {
                    lv.set(logging_page_view(st));
                } else {
                    lv.set(app_view(st));
                }
            }
        }
    }

    restore_input();

    // Restore screen + show cursor
    con.write("\x1b[?25h");
    con.write("\x1b[?1049l");

    std::printf("Thanks for trying the tapiru comprehensive TUI demo!\n");
    return 0;
}
