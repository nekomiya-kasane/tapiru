/**
 * @file vim_demo.cpp
 * @brief Vim-like text editor demo using classic_app + extended textarea_builder.
 *
 * Demonstrates:
 *   - Modal editing (Normal / Insert / Visual / Visual-Line / Command / Search)
 *   - Menu bar with File/Edit/View/Help
 *   - Status bar with mode indicator, filename, cursor position
 *   - Block cursor, selection highlighting, search match highlighting
 *   - Relative and absolute line numbers
 *   - Undo/redo stack
 *   - Vim keybindings (hjkl, w/b/e, dd, yy, p, o, etc.)
 *
 * Build:
 *   cmake --build build --target tapiru_vim_demo --config Release
 * Run:
 *   .\build\bin\Release\tapiru_vim_demo.exe
 */

#include "tapiru/widgets/classic_app.h"
#include "tapiru/widgets/textarea.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
//  Vim mode enum
// ═══════════════════════════════════════════════════════════════════════

enum class vim_mode : uint8_t {
    normal,
    insert,
    visual,
    visual_line,
    command,
    search,
};

// ═══════════════════════════════════════════════════════════════════════
//  Editor state
// ═══════════════════════════════════════════════════════════════════════

struct editor_state {
    // Document
    std::vector<std::string> lines;
    std::string filename = "[scratch]";
    bool modified = false;

    // Cursor
    int cursor_row = 0;
    int cursor_col = 0;

    // Mode
    vim_mode mode = vim_mode::normal;
    std::string command_buf;
    std::string status_msg;
    int status_msg_ticks = 0;

    // Visual mode
    int visual_anchor_row = 0;
    int visual_anchor_col = 0;

    // Search
    std::string search_pattern;
    std::vector<text_range> search_matches;
    int current_match = -1;

    // Yank register
    std::vector<std::string> yank_buf;
    bool yank_linewise = false;

    // Undo/redo
    struct snapshot {
        std::vector<std::string> lines;
        int row, col;
    };
    std::vector<snapshot> undo_stack;
    std::vector<snapshot> redo_stack;

    // Settings
    bool show_line_numbers = true;
    bool relative_numbers = false;

    // Pending operator
    bool g_pending = false;
    bool d_pending = false;
    bool y_pending = false;

    // Popup
    bool popup_open = false;
    std::string popup_title;
    std::vector<std::string> popup_lines;

    // Render-time cache (must outlive flatten_into)
    std::string render_text;
    int render_scroll = 0;
    text_range render_sel;
};

// ═══════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════

static void push_undo(editor_state &st) {
    st.undo_stack.push_back({st.lines, st.cursor_row, st.cursor_col});
    st.redo_stack.clear();
}

static void do_undo(editor_state &st) {
    if (st.undo_stack.empty()) {
        st.status_msg = "Already at oldest change";
        st.status_msg_ticks = 40;
        return;
    }
    st.redo_stack.push_back({st.lines, st.cursor_row, st.cursor_col});
    auto &snap = st.undo_stack.back();
    st.lines = std::move(snap.lines);
    st.cursor_row = snap.row;
    st.cursor_col = snap.col;
    st.undo_stack.pop_back();
    st.modified = true;
}

static void do_redo(editor_state &st) {
    if (st.redo_stack.empty()) {
        st.status_msg = "Already at newest change";
        st.status_msg_ticks = 40;
        return;
    }
    st.undo_stack.push_back({st.lines, st.cursor_row, st.cursor_col});
    auto &snap = st.redo_stack.back();
    st.lines = std::move(snap.lines);
    st.cursor_row = snap.row;
    st.cursor_col = snap.col;
    st.redo_stack.pop_back();
    st.modified = true;
}

static void clamp_cursor(editor_state &st) {
    if (st.lines.empty()) {
        st.lines.emplace_back("");
    }
    if (st.cursor_row < 0) {
        st.cursor_row = 0;
    }
    if (st.cursor_row >= static_cast<int>(st.lines.size())) {
        st.cursor_row = static_cast<int>(st.lines.size()) - 1;
    }
    int line_len = static_cast<int>(st.lines[static_cast<size_t>(st.cursor_row)].size());
    if (st.mode == vim_mode::normal || st.mode == vim_mode::visual || st.mode == vim_mode::visual_line) {
        if (line_len > 0 && st.cursor_col >= line_len) {
            st.cursor_col = line_len - 1;
        }
    }
    if (st.cursor_col < 0) {
        st.cursor_col = 0;
    }
}

static std::string join_lines(const std::vector<std::string> &lines) {
    std::string result;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            result += '\n';
        }
        result += lines[i];
    }
    return result;
}

static const char *mode_string(vim_mode m) {
    switch (m) {
    case vim_mode::normal:
        return "-- NORMAL --";
    case vim_mode::insert:
        return "-- INSERT --";
    case vim_mode::visual:
        return "-- VISUAL --";
    case vim_mode::visual_line:
        return "-- VISUAL LINE --";
    case vim_mode::command:
        return "";
    case vim_mode::search:
        return "";
    }
    return "";
}

static int find_word_forward(const std::string &line, int col) {
    int len = static_cast<int>(line.size());
    if (col >= len) {
        return col;
    }
    // Skip current word chars
    while (col < len && std::isalnum(static_cast<unsigned char>(line[static_cast<size_t>(col)]))) {
        ++col;
    }
    // Skip non-word chars
    while (col < len && !std::isalnum(static_cast<unsigned char>(line[static_cast<size_t>(col)]))) {
        ++col;
    }
    return col;
}

static int find_word_backward(const std::string &line, int col) {
    if (col <= 0) {
        return 0;
    }
    --col;
    // Skip non-word chars
    while (col > 0 && !std::isalnum(static_cast<unsigned char>(line[static_cast<size_t>(col)]))) {
        --col;
    }
    // Skip word chars
    while (col > 0 && std::isalnum(static_cast<unsigned char>(line[static_cast<size_t>(col - 1)]))) {
        --col;
    }
    return col;
}

static int find_word_end(const std::string &line, int col) {
    int len = static_cast<int>(line.size());
    if (col >= len - 1) {
        return len > 0 ? len - 1 : 0;
    }
    ++col;
    // Skip non-word chars
    while (col < len && !std::isalnum(static_cast<unsigned char>(line[static_cast<size_t>(col)]))) {
        ++col;
    }
    // Move to end of word
    while (col < len - 1 && std::isalnum(static_cast<unsigned char>(line[static_cast<size_t>(col + 1)]))) {
        ++col;
    }
    return col;
}

static void find_all_matches(editor_state &st) {
    st.search_matches.clear();
    if (st.search_pattern.empty()) {
        return;
    }
    for (int r = 0; r < static_cast<int>(st.lines.size()); ++r) {
        const auto &line = st.lines[static_cast<size_t>(r)];
        size_t pos = 0;
        while ((pos = line.find(st.search_pattern, pos)) != std::string::npos) {
            int cs = static_cast<int>(pos);
            int ce = cs + static_cast<int>(st.search_pattern.size());
            st.search_matches.push_back({r, cs, r, ce});
            pos += st.search_pattern.size();
            if (st.search_pattern.empty()) {
                break;
            }
        }
    }
}

static void jump_to_match(editor_state &st, int idx) {
    if (st.search_matches.empty()) {
        return;
    }
    int n = static_cast<int>(st.search_matches.size());
    idx = ((idx % n) + n) % n;
    st.current_match = idx;
    st.cursor_row = st.search_matches[static_cast<size_t>(idx)].start_row;
    st.cursor_col = st.search_matches[static_cast<size_t>(idx)].start_col;
}

static text_range compute_selection(const editor_state &st) {
    if (st.mode == vim_mode::visual) {
        return {st.visual_anchor_row, st.visual_anchor_col, st.cursor_row, st.cursor_col + 1};
    }
    if (st.mode == vim_mode::visual_line) {
        int sr = std::min(st.visual_anchor_row, st.cursor_row);
        int er = std::max(st.visual_anchor_row, st.cursor_row);
        int er_len = static_cast<int>(st.lines[static_cast<size_t>(er)].size());
        return {sr, 0, er, er_len > 0 ? er_len : 1};
    }
    return {};
}

static void yank_selection(editor_state &st) {
    auto sel = compute_selection(st);
    int sr = sel.start_row, sc = sel.start_col;
    int er = sel.end_row, ec = sel.end_col;
    if (sr > er || (sr == er && sc > ec)) {
        std::swap(sr, er);
        std::swap(sc, ec);
    }
    st.yank_buf.clear();
    st.yank_linewise = (st.mode == vim_mode::visual_line);
    if (st.yank_linewise) {
        for (int r = sr; r <= er; ++r) {
            st.yank_buf.push_back(st.lines[static_cast<size_t>(r)]);
        }
    } else {
        if (sr == er) {
            st.yank_buf.push_back(
                st.lines[static_cast<size_t>(sr)].substr(static_cast<size_t>(sc), static_cast<size_t>(ec - sc)));
        } else {
            st.yank_buf.push_back(st.lines[static_cast<size_t>(sr)].substr(static_cast<size_t>(sc)));
            for (int r = sr + 1; r < er; ++r) {
                st.yank_buf.push_back(st.lines[static_cast<size_t>(r)]);
            }
            st.yank_buf.push_back(st.lines[static_cast<size_t>(er)].substr(0, static_cast<size_t>(ec)));
        }
    }
    int count = st.yank_linewise ? static_cast<int>(st.yank_buf.size()) : 1;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d %s yanked", count, st.yank_linewise ? "lines" : "chars");
    st.status_msg = buf;
    st.status_msg_ticks = 40;
}

static void delete_selection(editor_state &st) {
    push_undo(st);
    auto sel = compute_selection(st);
    int sr = sel.start_row, sc = sel.start_col;
    int er = sel.end_row, ec = sel.end_col;
    if (sr > er || (sr == er && sc > ec)) {
        std::swap(sr, er);
        std::swap(sc, ec);
    }
    if (st.mode == vim_mode::visual_line) {
        st.lines.erase(st.lines.begin() + sr, st.lines.begin() + er + 1);
        if (st.lines.empty()) {
            st.lines.emplace_back("");
        }
        st.cursor_row = sr;
    } else {
        std::string before = st.lines[static_cast<size_t>(sr)].substr(0, static_cast<size_t>(sc));
        std::string after = st.lines[static_cast<size_t>(er)].substr(static_cast<size_t>(ec));
        st.lines[static_cast<size_t>(sr)] = before + after;
        if (er > sr) {
            st.lines.erase(st.lines.begin() + sr + 1, st.lines.begin() + er + 1);
        }
        st.cursor_row = sr;
        st.cursor_col = sc;
    }
    st.modified = true;
}

// ═══════════════════════════════════════════════════════════════════════
//  Menu definitions
// ═══════════════════════════════════════════════════════════════════════

static std::vector<menu_bar_entry> build_menus() {
    std::vector<menu_bar_entry> menus;

    menus.push_back({"File",
                     {
                         menu_item_builder("New").shortcut("Ctrl+N"),
                         menu_item_builder("Open Sample"),
                         menu_item_builder("Open Recent")
                             .add(menu_item_builder("sample.cpp"))
                             .add(menu_item_builder("main.cpp"))
                             .add(menu_item_builder("config.h"))
                             .add(menu_item_builder::separator())
                             .add(menu_item_builder("Clear Recent")),
                         menu_item_builder::separator(),
                         menu_item_builder("Save").shortcut(":w"),
                         menu_item_builder("Save & Quit").shortcut(":wq"),
                         menu_item_builder::separator(),
                         menu_item_builder("Quit").shortcut(":q"),
                     }});

    menus.push_back({"Edit",
                     {
                         menu_item_builder("Undo").shortcut("u"),
                         menu_item_builder("Redo").shortcut("Ctrl+R"),
                         menu_item_builder::separator(),
                         menu_item_builder("Cut").shortcut("d (visual)"),
                         menu_item_builder("Copy").shortcut("y (visual)"),
                         menu_item_builder("Paste After").shortcut("p"),
                         menu_item_builder("Paste Before").shortcut("P"),
                         menu_item_builder::separator(),
                         menu_item_builder("Preferences")
                             .add(menu_item_builder("Color Theme")
                                      .add(menu_item_builder("Dark (default)"))
                                      .add(menu_item_builder("Light"))
                                      .add(menu_item_builder("Solarized")))
                             .add(menu_item_builder("Font Size")
                                      .add(menu_item_builder("Small"))
                                      .add(menu_item_builder("Medium"))
                                      .add(menu_item_builder("Large"))),
                     }});

    menus.push_back({"View",
                     {
                         menu_item_builder("Toggle Line Numbers").shortcut(":set number"),
                         menu_item_builder("Toggle Relative Numbers").shortcut(":set rnu"),
                         menu_item_builder::separator(),
                         menu_item_builder("Go to Top").shortcut("gg"),
                         menu_item_builder("Go to Bottom").shortcut("G"),
                     }});

    menus.push_back({"Help",
                     {
                         menu_item_builder("Keybindings..."),
                         menu_item_builder("Quick Reference")
                             .add(menu_item_builder("Normal Mode")
                                      .add(menu_item_builder("hjkl  Move cursor"))
                                      .add(menu_item_builder("w/b   Word forward/back"))
                                      .add(menu_item_builder("0/$   Line start/end"))
                                      .add(menu_item_builder("gg/G  File start/end"))
                                      .add(menu_item_builder("dd    Delete line"))
                                      .add(menu_item_builder("yy    Yank line"))
                                      .add(menu_item_builder("p/P   Paste after/before")))
                             .add(menu_item_builder("Insert Mode")
                                      .add(menu_item_builder("i/a   Insert/Append"))
                                      .add(menu_item_builder("I/A   Line start/end"))
                                      .add(menu_item_builder("o/O   Open line below/above"))
                                      .add(menu_item_builder("Esc   Back to Normal")))
                             .add(menu_item_builder("Visual Mode")
                                      .add(menu_item_builder("v/V   Char/Line select"))
                                      .add(menu_item_builder("d     Delete selected"))
                                      .add(menu_item_builder("y     Yank selected"))),
                         menu_item_builder::separator(),
                         menu_item_builder("About tapiru..."),
                     }});

    return menus;
}

// ═══════════════════════════════════════════════════════════════════════
//  Sample document
// ═══════════════════════════════════════════════════════════════════════

static std::vector<std::string> sample_document() {
    return {
        "#include <iostream>",
        "#include <string>",
        "#include <vector>",
        "",
        "// A simple C++ program demonstrating tapiru's vim-like editor",
        "",
        "struct Point {",
        "    double x = 0.0;",
        "    double y = 0.0;",
        "",
        "    double distance() const {",
        "        return std::sqrt(x * x + y * y);",
        "    }",
        "};",
        "",
        "int main() {",
        "    std::string name = \"tapiru\";",
        "    std::cout << \"Hello, \" << name << \"!\" << std::endl;",
        "",
        "    std::vector<Point> points = {",
        "        {1.0, 2.0},",
        "        {3.0, 4.0},",
        "        {5.0, 6.0},",
        "    };",
        "",
        "    for (const auto& p : points) {",
        "        std::cout << \"Distance: \" << p.distance() << std::endl;",
        "    }",
        "",
        "    return 0;",
        "}",
    };
}

// ═══════════════════════════════════════════════════════════════════════
//  Command parser
// ═══════════════════════════════════════════════════════════════════════

static bool execute_command(editor_state &st, classic_app &app, const std::string &cmd) {
    if (cmd == "q" || cmd == "q!") {
        app.quit();
        return true;
    }
    if (cmd == "w") {
        st.status_msg = "\"" + st.filename + "\" written (simulated)";
        st.status_msg_ticks = 40;
        return true;
    }
    if (cmd == "wq") {
        app.quit();
        return true;
    }
    if (cmd == "set number") {
        st.show_line_numbers = true;
        st.status_msg = "Line numbers ON";
        st.status_msg_ticks = 30;
        return true;
    }
    if (cmd == "set nonumber" || cmd == "set nonu") {
        st.show_line_numbers = false;
        st.status_msg = "Line numbers OFF";
        st.status_msg_ticks = 30;
        return true;
    }
    if (cmd == "set relativenumber" || cmd == "set rnu") {
        st.relative_numbers = true;
        st.status_msg = "Relative numbers ON";
        st.status_msg_ticks = 30;
        return true;
    }
    if (cmd == "set norelativenumber" || cmd == "set nornu") {
        st.relative_numbers = false;
        st.status_msg = "Relative numbers OFF";
        st.status_msg_ticks = 30;
        return true;
    }
    // :{number} — go to line
    bool all_digits = !cmd.empty();
    for (char c : cmd) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            all_digits = false;
            break;
        }
    }
    if (all_digits) {
        int line = std::atoi(cmd.c_str()) - 1;
        if (line < 0) {
            line = 0;
        }
        st.cursor_row = line;
        st.cursor_col = 0;
        clamp_cursor(st);
        return true;
    }
    st.status_msg = "E492: Not an editor command: " + cmd;
    st.status_msg_ticks = 40;
    return false;
}

// ═══════════════════════════════════════════════════════════════════════
//  Input handling
// ═══════════════════════════════════════════════════════════════════════

static bool handle_normal(editor_state &st, classic_app &app, const key_event &ke) {
    int total = static_cast<int>(st.lines.size());
    int line_len = static_cast<int>(st.lines[static_cast<size_t>(st.cursor_row)].size());

    // Pending operators
    if (st.g_pending) {
        st.g_pending = false;
        if (ke.codepoint == 'g') {
            st.cursor_row = 0;
            st.cursor_col = 0;
            clamp_cursor(st);
            return true;
        }
        return true;
    }
    if (st.d_pending) {
        st.d_pending = false;
        if (ke.codepoint == 'd') {
            push_undo(st);
            st.yank_buf = {st.lines[static_cast<size_t>(st.cursor_row)]};
            st.yank_linewise = true;
            st.lines.erase(st.lines.begin() + st.cursor_row);
            if (st.lines.empty()) {
                st.lines.emplace_back("");
            }
            clamp_cursor(st);
            st.modified = true;
            st.status_msg = "1 line deleted";
            st.status_msg_ticks = 30;
            return true;
        }
        return true;
    }
    if (st.y_pending) {
        st.y_pending = false;
        if (ke.codepoint == 'y') {
            st.yank_buf = {st.lines[static_cast<size_t>(st.cursor_row)]};
            st.yank_linewise = true;
            st.status_msg = "1 line yanked";
            st.status_msg_ticks = 30;
            return true;
        }
        return true;
    }

    // Movement
    if (ke.codepoint == 'h' || ke.key == special_key::left) {
        if (st.cursor_col > 0) {
            --st.cursor_col;
        }
        return true;
    }
    if (ke.codepoint == 'l' || ke.key == special_key::right) {
        if (st.cursor_col < line_len - 1) {
            ++st.cursor_col;
        }
        if (st.cursor_col < 0) {
            st.cursor_col = 0;
        }
        return true;
    }
    if (ke.codepoint == 'j' || ke.key == special_key::down) {
        if (st.cursor_row < total - 1) {
            ++st.cursor_row;
        }
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'k' || ke.key == special_key::up) {
        if (st.cursor_row > 0) {
            --st.cursor_row;
        }
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'w') {
        st.cursor_col = find_word_forward(st.lines[static_cast<size_t>(st.cursor_row)], st.cursor_col);
        if (st.cursor_col >= line_len && st.cursor_row < total - 1) {
            ++st.cursor_row;
            st.cursor_col = 0;
        }
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'b') {
        if (st.cursor_col == 0 && st.cursor_row > 0) {
            --st.cursor_row;
            st.cursor_col = static_cast<int>(st.lines[static_cast<size_t>(st.cursor_row)].size());
        }
        st.cursor_col = find_word_backward(st.lines[static_cast<size_t>(st.cursor_row)], st.cursor_col);
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'e') {
        st.cursor_col = find_word_end(st.lines[static_cast<size_t>(st.cursor_row)], st.cursor_col);
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == '0') {
        st.cursor_col = 0;
        return true;
    }
    if (ke.codepoint == '$') {
        st.cursor_col = line_len > 0 ? line_len - 1 : 0;
        return true;
    }
    if (ke.codepoint == 'G') {
        st.cursor_row = total - 1;
        st.cursor_col = 0;
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'g') {
        st.g_pending = true;
        return true;
    }
    if (ke.codepoint == 'H') {
        st.cursor_row = app.scroll_y();
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'M') {
        st.cursor_row = app.scroll_y() + app.viewport_h() / 2;
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'L') {
        st.cursor_row = app.scroll_y() + app.viewport_h() - 1;
        clamp_cursor(st);
        return true;
    }

    // Half-page scroll
    if (ke.key == special_key::none && ke.codepoint == 4) { // Ctrl+D
        st.cursor_row += app.viewport_h() / 2;
        clamp_cursor(st);
        return true;
    }
    if (ke.key == special_key::none && ke.codepoint == 21) { // Ctrl+U
        st.cursor_row -= app.viewport_h() / 2;
        clamp_cursor(st);
        return true;
    }

    // Mode switches
    if (ke.codepoint == 'i') {
        st.mode = vim_mode::insert;
        return true;
    }
    if (ke.codepoint == 'a') {
        st.mode = vim_mode::insert;
        if (line_len > 0) {
            ++st.cursor_col;
        }
        return true;
    }
    if (ke.codepoint == 'I') {
        st.mode = vim_mode::insert;
        st.cursor_col = 0;
        return true;
    }
    if (ke.codepoint == 'A') {
        st.mode = vim_mode::insert;
        st.cursor_col = line_len;
        return true;
    }
    if (ke.codepoint == 'o') {
        push_undo(st);
        st.lines.insert(st.lines.begin() + st.cursor_row + 1, "");
        ++st.cursor_row;
        st.cursor_col = 0;
        st.mode = vim_mode::insert;
        st.modified = true;
        return true;
    }
    if (ke.codepoint == 'O') {
        push_undo(st);
        st.lines.insert(st.lines.begin() + st.cursor_row, "");
        st.cursor_col = 0;
        st.mode = vim_mode::insert;
        st.modified = true;
        return true;
    }
    if (ke.codepoint == 'v') {
        st.mode = vim_mode::visual;
        st.visual_anchor_row = st.cursor_row;
        st.visual_anchor_col = st.cursor_col;
        return true;
    }
    if (ke.codepoint == 'V') {
        st.mode = vim_mode::visual_line;
        st.visual_anchor_row = st.cursor_row;
        st.visual_anchor_col = 0;
        return true;
    }
    if (ke.codepoint == ':') {
        st.mode = vim_mode::command;
        st.command_buf.clear();
        return true;
    }
    if (ke.codepoint == '/') {
        st.mode = vim_mode::search;
        st.command_buf.clear();
        return true;
    }

    // Editing
    if (ke.codepoint == 'x') {
        if (line_len > 0) {
            push_undo(st);
            auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
            line.erase(static_cast<size_t>(st.cursor_col), 1);
            clamp_cursor(st);
            st.modified = true;
        }
        return true;
    }
    if (ke.codepoint == 'd') {
        st.d_pending = true;
        return true;
    }
    if (ke.codepoint == 'y') {
        st.y_pending = true;
        return true;
    }
    if (ke.codepoint == 'p') {
        if (!st.yank_buf.empty()) {
            push_undo(st);
            if (st.yank_linewise) {
                int insert_at = st.cursor_row + 1;
                for (size_t i = 0; i < st.yank_buf.size(); ++i) {
                    st.lines.insert(st.lines.begin() + insert_at + static_cast<int>(i), st.yank_buf[i]);
                }
                st.cursor_row = insert_at;
                st.cursor_col = 0;
            } else {
                auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
                int pos = st.cursor_col + 1;
                if (pos > static_cast<int>(line.size())) {
                    pos = static_cast<int>(line.size());
                }
                line.insert(static_cast<size_t>(pos), st.yank_buf[0]);
                st.cursor_col = pos + static_cast<int>(st.yank_buf[0].size()) - 1;
            }
            st.modified = true;
            clamp_cursor(st);
        }
        return true;
    }
    if (ke.codepoint == 'P') {
        if (!st.yank_buf.empty()) {
            push_undo(st);
            if (st.yank_linewise) {
                for (size_t i = 0; i < st.yank_buf.size(); ++i) {
                    st.lines.insert(st.lines.begin() + st.cursor_row + static_cast<int>(i), st.yank_buf[i]);
                }
                st.cursor_col = 0;
            } else {
                auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
                line.insert(static_cast<size_t>(st.cursor_col), st.yank_buf[0]);
            }
            st.modified = true;
            clamp_cursor(st);
        }
        return true;
    }
    if (ke.codepoint == 'u') {
        do_undo(st);
        return true;
    }
    if (ke.key == special_key::none && ke.codepoint == 18) { // Ctrl+R
        do_redo(st);
        return true;
    }

    // Search navigation
    if (ke.codepoint == 'n') {
        if (!st.search_matches.empty()) {
            jump_to_match(st, st.current_match + 1);
        }
        return true;
    }
    if (ke.codepoint == 'N') {
        if (!st.search_matches.empty()) {
            jump_to_match(st, st.current_match - 1);
        }
        return true;
    }

    if (ke.key == special_key::escape) {
        st.search_matches.clear();
        st.search_pattern.clear();
        return true;
    }

    (void)app;
    // Consume all keys in normal mode to prevent classic_app defaults (q=quit, Escape=quit)
    return true;
}

static bool handle_insert(editor_state &st, const key_event &ke) {
    if (ke.key == special_key::escape) {
        push_undo(st);
        st.mode = vim_mode::normal;
        if (st.cursor_col > 0) {
            --st.cursor_col;
        }
        clamp_cursor(st);
        return true;
    }

    // Arrow keys
    if (ke.key == special_key::up) {
        if (st.cursor_row > 0) {
            --st.cursor_row;
        }
        clamp_cursor(st);
        return true;
    }
    if (ke.key == special_key::down) {
        if (st.cursor_row < static_cast<int>(st.lines.size()) - 1) {
            ++st.cursor_row;
        }
        clamp_cursor(st);
        return true;
    }
    if (ke.key == special_key::left) {
        if (st.cursor_col > 0) {
            --st.cursor_col;
        }
        return true;
    }
    if (ke.key == special_key::right) {
        ++st.cursor_col;
        clamp_cursor(st);
        return true;
    }

    if (ke.key == special_key::backspace) {
        auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
        if (st.cursor_col > 0) {
            line.erase(static_cast<size_t>(st.cursor_col - 1), 1);
            --st.cursor_col;
        } else if (st.cursor_row > 0) {
            // Join with previous line
            auto &prev = st.lines[static_cast<size_t>(st.cursor_row - 1)];
            st.cursor_col = static_cast<int>(prev.size());
            prev += line;
            st.lines.erase(st.lines.begin() + st.cursor_row);
            --st.cursor_row;
        }
        st.modified = true;
        return true;
    }

    if (ke.key == special_key::delete_) {
        auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
        int len = static_cast<int>(line.size());
        if (st.cursor_col < len) {
            line.erase(static_cast<size_t>(st.cursor_col), 1);
        } else if (st.cursor_row < static_cast<int>(st.lines.size()) - 1) {
            line += st.lines[static_cast<size_t>(st.cursor_row + 1)];
            st.lines.erase(st.lines.begin() + st.cursor_row + 1);
        }
        st.modified = true;
        return true;
    }

    if (ke.key == special_key::enter) {
        auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
        std::string rest = line.substr(static_cast<size_t>(st.cursor_col));
        line.erase(static_cast<size_t>(st.cursor_col));
        st.lines.insert(st.lines.begin() + st.cursor_row + 1, rest);
        ++st.cursor_row;
        st.cursor_col = 0;
        st.modified = true;
        return true;
    }

    if (ke.key == special_key::tab) {
        auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
        line.insert(static_cast<size_t>(st.cursor_col), "    ");
        st.cursor_col += 4;
        st.modified = true;
        return true;
    }

    // Printable character
    if (ke.codepoint >= 32 && ke.codepoint < 127) {
        auto &line = st.lines[static_cast<size_t>(st.cursor_row)];
        if (st.cursor_col > static_cast<int>(line.size())) {
            st.cursor_col = static_cast<int>(line.size());
        }
        line.insert(line.begin() + st.cursor_col, static_cast<char>(ke.codepoint));
        ++st.cursor_col;
        st.modified = true;
        return true;
    }

    return false;
}

static bool handle_visual(editor_state &st, const key_event &ke) {
    if (ke.key == special_key::escape) {
        st.mode = vim_mode::normal;
        return true;
    }

    int total = static_cast<int>(st.lines.size());
    int line_len = static_cast<int>(st.lines[static_cast<size_t>(st.cursor_row)].size());

    // Movement (same as normal)
    if (ke.codepoint == 'h' || ke.key == special_key::left) {
        if (st.cursor_col > 0) {
            --st.cursor_col;
        }
        return true;
    }
    if (ke.codepoint == 'l' || ke.key == special_key::right) {
        if (st.cursor_col < line_len - 1) {
            ++st.cursor_col;
        }
        return true;
    }
    if (ke.codepoint == 'j' || ke.key == special_key::down) {
        if (st.cursor_row < total - 1) {
            ++st.cursor_row;
        }
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'k' || ke.key == special_key::up) {
        if (st.cursor_row > 0) {
            --st.cursor_row;
        }
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'w') {
        st.cursor_col = find_word_forward(st.lines[static_cast<size_t>(st.cursor_row)], st.cursor_col);
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'b') {
        st.cursor_col = find_word_backward(st.lines[static_cast<size_t>(st.cursor_row)], st.cursor_col);
        return true;
    }
    if (ke.codepoint == '0') {
        st.cursor_col = 0;
        return true;
    }
    if (ke.codepoint == '$') {
        st.cursor_col = line_len > 0 ? line_len - 1 : 0;
        return true;
    }
    if (ke.codepoint == 'G') {
        st.cursor_row = total - 1;
        clamp_cursor(st);
        return true;
    }

    // Switch visual modes
    if (ke.codepoint == 'v' && st.mode == vim_mode::visual_line) {
        st.mode = vim_mode::visual;
        return true;
    }
    if (ke.codepoint == 'V' && st.mode == vim_mode::visual) {
        st.mode = vim_mode::visual_line;
        return true;
    }

    // Operations
    if (ke.codepoint == 'd') {
        yank_selection(st);
        delete_selection(st);
        st.mode = vim_mode::normal;
        clamp_cursor(st);
        return true;
    }
    if (ke.codepoint == 'y') {
        yank_selection(st);
        st.mode = vim_mode::normal;
        return true;
    }

    return false;
}

static bool handle_command(editor_state &st, classic_app &app, const key_event &ke) {
    if (ke.key == special_key::escape) {
        st.mode = vim_mode::normal;
        st.command_buf.clear();
        return true;
    }
    if (ke.key == special_key::enter) {
        execute_command(st, app, st.command_buf);
        st.mode = vim_mode::normal;
        st.command_buf.clear();
        return true;
    }
    if (ke.key == special_key::backspace) {
        if (st.command_buf.empty()) {
            st.mode = vim_mode::normal;
        } else {
            st.command_buf.pop_back();
        }
        return true;
    }
    if (ke.codepoint >= 32 && ke.codepoint < 127) {
        st.command_buf += static_cast<char>(ke.codepoint);
        return true;
    }
    return false;
}

static bool handle_search(editor_state &st, const key_event &ke) {
    if (ke.key == special_key::escape) {
        st.mode = vim_mode::normal;
        st.command_buf.clear();
        st.search_matches.clear();
        st.search_pattern.clear();
        return true;
    }
    if (ke.key == special_key::enter) {
        st.search_pattern = st.command_buf;
        find_all_matches(st);
        if (!st.search_matches.empty()) {
            // Jump to first match at or after cursor
            int best = 0;
            for (int i = 0; i < static_cast<int>(st.search_matches.size()); ++i) {
                const auto &m = st.search_matches[static_cast<size_t>(i)];
                if (m.start_row > st.cursor_row || (m.start_row == st.cursor_row && m.start_col >= st.cursor_col)) {
                    best = i;
                    break;
                }
            }
            jump_to_match(st, best);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "/%s  [%d/%d]", st.search_pattern.c_str(), st.current_match + 1,
                          static_cast<int>(st.search_matches.size()));
            st.status_msg = buf;
            st.status_msg_ticks = 40;
        } else {
            st.status_msg = "Pattern not found: " + st.search_pattern;
            st.status_msg_ticks = 40;
        }
        st.mode = vim_mode::normal;
        st.command_buf.clear();
        return true;
    }
    if (ke.key == special_key::backspace) {
        if (st.command_buf.empty()) {
            st.mode = vim_mode::normal;
            st.search_matches.clear();
        } else {
            st.command_buf.pop_back();
            // Live update matches
            st.search_pattern = st.command_buf;
            find_all_matches(st);
        }
        return true;
    }
    if (ke.codepoint >= 32 && ke.codepoint < 127) {
        st.command_buf += static_cast<char>(ke.codepoint);
        // Live update matches
        st.search_pattern = st.command_buf;
        find_all_matches(st);
        return true;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════
//  Menu action handler
// ═══════════════════════════════════════════════════════════════════════

static void handle_menu_action(editor_state &st, classic_app &app, const std::string &label) {
    if (label == "Quit") {
        app.quit();
        return;
    }
    if (label == "Save & Quit") {
        app.quit();
        return;
    }
    if (label == "Save") {
        st.status_msg = "\"" + st.filename + "\" written (simulated)";
        st.status_msg_ticks = 40;
        return;
    }
    if (label == "New") {
        push_undo(st);
        st.lines = {""};
        st.cursor_row = 0;
        st.cursor_col = 0;
        st.filename = "[scratch]";
        st.modified = false;
        st.status_msg = "New buffer";
        st.status_msg_ticks = 30;
        return;
    }
    if (label == "Open Sample") {
        push_undo(st);
        st.lines = sample_document();
        st.cursor_row = 0;
        st.cursor_col = 0;
        st.filename = "sample.cpp";
        st.modified = false;
        st.status_msg = "\"sample.cpp\" loaded";
        st.status_msg_ticks = 30;
        return;
    }
    if (label == "Undo") {
        do_undo(st);
        return;
    }
    if (label == "Redo") {
        do_redo(st);
        return;
    }
    if (label == "Toggle Line Numbers") {
        st.show_line_numbers = !st.show_line_numbers;
        return;
    }
    if (label == "Toggle Relative Numbers") {
        st.relative_numbers = !st.relative_numbers;
        return;
    }
    if (label == "Go to Top") {
        st.cursor_row = 0;
        st.cursor_col = 0;
        return;
    }
    if (label == "Go to Bottom") {
        st.cursor_row = static_cast<int>(st.lines.size()) - 1;
        st.cursor_col = 0;
        clamp_cursor(st);
        return;
    }
    if (label == "Keybindings...") {
        st.popup_open = true;
        st.popup_title = " Keybindings ";
        st.popup_lines = {
            "",
            "  NORMAL MODE                          ",
            "  h/j/k/l .... Move left/down/up/right ",
            "  w / b ...... Next / prev word         ",
            "  e .......... End of word              ",
            "  0 / $ ...... Line start / end         ",
            "  gg / G ..... File start / end         ",
            "  H / M / L .. Screen top/mid/bottom    ",
            "  Ctrl+D / U . Half-page down / up      ",
            "",
            "  INSERT MODE                          ",
            "  i / a ...... Insert / Append          ",
            "  I / A ...... Line start / end         ",
            "  o / O ...... Open line below / above  ",
            "  Esc ........ Back to Normal           ",
            "",
            "  VISUAL MODE                          ",
            "  v / V ...... Char / Line select       ",
            "  d .......... Delete selected          ",
            "  y .......... Yank selected            ",
            "",
            "  EDITING                              ",
            "  x .......... Delete char              ",
            "  dd ......... Delete line              ",
            "  yy ......... Yank line                ",
            "  p / P ...... Paste after / before     ",
            "  u .......... Undo                     ",
            "  Ctrl+R ..... Redo                     ",
            "",
            "  COMMAND / SEARCH                     ",
            "  : .......... Command mode             ",
            "  / .......... Search mode              ",
            "  n / N ...... Next / prev match        ",
            "",
            "        Press Esc to close              ",
            "",
        };
        return;
    }
    if (label == "About tapiru...") {
        st.popup_open = true;
        st.popup_title = " About ";
        st.popup_lines = {
            "",
            "     tapiru vim demo                ",
            "",
            "  A vim-like text editor built with ",
            "  the tapiru terminal UI library.   ",
            "",
            "  Features:                         ",
            "   - Modal editing (N/I/V/C/S)      ",
            "   - Menu bar with cascading menus   ",
            "   - Block & underline cursors       ",
            "   - Visual selection highlighting   ",
            "   - Live search with highlighting   ",
            "   - Relative line numbers           ",
            "   - Undo / redo stack               ",
            "",
            "  github.com/user/tapiru            ",
            "",
            "        Press Esc to close           ",
            "",
        };
        return;
    }
    st.status_msg = "Menu: " + label;
    st.status_msg_ticks = 30;
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    editor_state st;
    st.lines = sample_document();
    st.filename = "sample.cpp";

    classic_app app(classic_app::config{
        .menus = build_menus(),
        .theme = classic_app_theme::dark(),
    });

    app.set_content_lines(static_cast<int>(st.lines.size()));

    // Styles
    style text_sty{color::from_rgb(204, 204, 204), color::default_color()};
    style cursor_line_sty{color::from_rgb(204, 204, 204), color::from_rgb(30, 30, 50)};
    style gutter_sty{color::from_rgb(100, 100, 100), color::default_color()};
    style block_cursor_sty{color::from_rgb(0, 0, 0), color::from_rgb(200, 200, 200)};
    style insert_cursor_sty{color::from_rgb(200, 200, 200), color::default_color(), attr::underline};
    style sel_sty{color::from_rgb(255, 255, 255), color::from_rgb(38, 79, 120)};
    style match_sty{color::from_rgb(0, 0, 0), color::from_rgb(255, 200, 50)};
    style eof_sty{color::from_rgb(60, 60, 100), color::default_color()};

    app.set_content([&](rows_builder &content, int scroll_y, int viewport_h) {
        // Cache into editor_state so pointers survive until flatten_into runs
        st.render_text = join_lines(st.lines);
        st.render_scroll = scroll_y;

        textarea_builder ta;
        ta.text(&st.render_text)
            .cursor_row(&st.cursor_row)
            .cursor_col(&st.cursor_col)
            .scroll_row(&st.render_scroll)
            .width(app.term_width() > 8 ? app.term_width() - 8 : 40)
            .height(static_cast<uint32_t>(viewport_h))
            .text_style(text_sty)
            .cursor_style(cursor_line_sty)
            .line_number_style(gutter_sty)
            .show_cursor(st.mode != vim_mode::command && st.mode != vim_mode::search)
            .cursor_char_style(st.mode == vim_mode::insert ? insert_cursor_sty : block_cursor_sty)
            .eof_style(eof_sty)
            .border(border_style::none);

        if (st.show_line_numbers || st.relative_numbers) {
            ta.show_line_numbers();
        }
        if (st.relative_numbers) {
            ta.relative_line_numbers();
        }

        // Selection
        if (st.mode == vim_mode::visual || st.mode == vim_mode::visual_line) {
            st.render_sel = compute_selection(st);
            ta.selection(&st.render_sel).selection_style(sel_sty);
        }

        // Search matches
        if (!st.search_matches.empty()) {
            ta.matches(&st.search_matches).match_style(match_sty);
        }

        content.add(std::move(ta));
    });

    app.set_status([&](status_bar_builder &sb) {
        // Tick status message
        if (st.status_msg_ticks > 0) {
            --st.status_msg_ticks;
        }

        // Left: mode or command/search prompt
        if (st.mode == vim_mode::command) {
            std::string left = ":" + st.command_buf;
            sb.left(left);
        } else if (st.mode == vim_mode::search) {
            std::string left = "/" + st.command_buf;
            sb.left(left);
        } else if (st.status_msg_ticks > 0) {
            sb.left(st.status_msg);
        } else {
            sb.left(mode_string(st.mode));
        }

        // Center: filename
        std::string center = st.filename;
        if (st.modified) {
            center += " [+]";
        }
        sb.center(center);

        // Right: cursor position
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Ln %d, Col %d  UTF-8", st.cursor_row + 1, st.cursor_col + 1);
        sb.right(buf);
    });

    app.on_menu_action([&](int, int, const std::string &label) { handle_menu_action(st, app, label); });

    // Popup window overlay (rendered as raw ANSI on top of the content area)
    app.set_overlay([&](std::string &buf, uint32_t tw, uint32_t th, int /*scroll_y*/, int /*viewport_h*/) {
        if (!st.popup_open || st.popup_lines.empty()) {
            return;
        }

        // Compute popup dimensions
        int content_w = 0;
        for (const auto &line : st.popup_lines) {
            int w = static_cast<int>(line.size());
            if (w > content_w) {
                content_w = w;
            }
        }
        int box_w = content_w + 4;                               // 2 border + 2 padding
        int box_h = static_cast<int>(st.popup_lines.size()) + 2; // 2 border rows
        int title_w = static_cast<int>(st.popup_title.size());
        if (title_w + 4 > box_w) {
            box_w = title_w + 4;
        }

        // Center on screen
        int x0 = (static_cast<int>(tw) - box_w) / 2;
        int y0 = (static_cast<int>(th) - box_h) / 2;
        if (x0 < 1) {
            x0 = 1;
        }
        if (y0 < 1) {
            y0 = 1;
        }

        // Colors: bright border, dark background
        const char *border_style = "\x1b[38;2;100;149;237m\x1b[48;2;30;30;40m";  // cornflower blue on dark
        const char *title_style = "\x1b[1;38;2;255;255;255m\x1b[48;2;30;30;40m"; // bold white on dark
        const char *text_style = "\x1b[0;38;2;200;200;200m\x1b[48;2;30;30;40m";  // gray on dark
        const char *reset = "\x1b[0m";

        // Top border with title
        char pos[32];
        std::snprintf(pos, sizeof(pos), "\x1b[%d;%dH", y0, x0);
        buf += pos;
        buf += border_style;
        buf += "\xe2\x95\x94"; // ╔
        // Title centered in top border
        int fill_before = (box_w - 2 - title_w) / 2;
        int fill_after = box_w - 2 - title_w - fill_before;
        for (int i = 0; i < fill_before; ++i) {
            buf += "\xe2\x95\x90"; // ═
        }
        buf += title_style;
        buf += st.popup_title;
        buf += border_style;
        for (int i = 0; i < fill_after; ++i) {
            buf += "\xe2\x95\x90"; // ═
        }
        buf += "\xe2\x95\x97"; // ╗

        // Content lines
        for (int r = 0; r < static_cast<int>(st.popup_lines.size()); ++r) {
            std::snprintf(pos, sizeof(pos), "\x1b[%d;%dH", y0 + 1 + r, x0);
            buf += pos;
            buf += border_style;
            buf += "\xe2\x95\x91"; // ║
            buf += text_style;
            buf += " ";
            const auto &line = st.popup_lines[static_cast<size_t>(r)];
            buf += line;
            int pad = content_w - static_cast<int>(line.size());
            for (int i = 0; i < pad; ++i) {
                buf += ' ';
            }
            buf += " ";
            buf += border_style;
            buf += "\xe2\x95\x91"; // ║
        }

        // Bottom border
        std::snprintf(pos, sizeof(pos), "\x1b[%d;%dH", y0 + box_h - 1, x0);
        buf += pos;
        buf += border_style;
        buf += "\xe2\x95\x9a"; // ╚
        for (int i = 0; i < box_w - 2; ++i) {
            buf += "\xe2\x95\x90"; // ═
        }
        buf += "\xe2\x95\x9d"; // ╝
        buf += reset;

        // Drop shadow (right edge)
        for (int r = 1; r < box_h; ++r) {
            std::snprintf(pos, sizeof(pos), "\x1b[%d;%dH", y0 + r, x0 + box_w);
            buf += pos;
            buf += "\x1b[38;2;40;40;40m\xe2\x96\x88\x1b[0m"; // dark block
        }
        // Drop shadow (bottom edge)
        std::snprintf(pos, sizeof(pos), "\x1b[%d;%dH", y0 + box_h, x0 + 1);
        buf += pos;
        for (int i = 0; i < box_w; ++i) {
            buf += "\x1b[38;2;40;40;40m\xe2\x96\x84\x1b[0m"; // ▄
        }
    });

    app.on_key([&](const key_event &ke) -> bool {
        // Popup intercepts all keys; Escape closes it
        if (st.popup_open) {
            if (ke.key == special_key::escape || ke.codepoint == 'q' || ke.key == special_key::enter) {
                st.popup_open = false;
                st.popup_lines.clear();
            }
            return true; // consume all keys while popup is open
        }

        bool handled = false;
        switch (st.mode) {
        case vim_mode::normal:
            handled = handle_normal(st, app, ke);
            break;
        case vim_mode::insert:
            handled = handle_insert(st, ke);
            break;
        case vim_mode::visual:
        case vim_mode::visual_line:
            handled = handle_visual(st, ke);
            break;
        case vim_mode::command:
            handled = handle_command(st, app, ke);
            break;
        case vim_mode::search:
            handled = handle_search(st, ke);
            break;
        }

        // Keep scroll in sync
        app.set_content_lines(static_cast<int>(st.lines.size()));

        // Ensure cursor is visible
        int scroll = app.scroll_y();
        int vh = app.viewport_h();
        if (st.cursor_row < scroll) {
            app.scroll_to(st.cursor_row);
        } else if (st.cursor_row >= scroll + vh) {
            app.scroll_to(st.cursor_row - vh + 1);
        }

        return handled;
    });

    app.run();

    std::printf("Thanks for trying the vim demo!\n");
    return 0;
}
