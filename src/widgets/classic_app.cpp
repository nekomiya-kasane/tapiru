/**
 * @file classic_app.cpp
 * @brief Implementation of the classic_app full-screen terminal application shell.
 */

#include "tapiru/widgets/classic_app.h"

#include "tapiru/core/console.h"
#include "tapiru/core/raw_input.h"
#include "tapiru/core/terminal.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace tapiru {

// ═══════════════════════════════════════════════════════════════════════
//  Theme presets
// ═══════════════════════════════════════════════════════════════════════

classic_app_theme classic_app_theme::dark() {
    return {
        // menu_bar_bg
        style{color::from_rgb(204, 204, 204), color::from_rgb(51, 51, 51)},
        // menu_bar_active
        style{color::from_rgb(255, 255, 255), color::from_rgb(4, 57, 94)},
        // dropdown_border
        style{color::from_rgb(69, 69, 69), color::from_rgb(37, 37, 38)},
        // dropdown_item
        style{color::from_rgb(204, 204, 204), color::from_rgb(37, 37, 38)},
        // dropdown_highlight
        style{color::from_rgb(255, 255, 255), color::from_rgb(4, 57, 94)},
        // dropdown_shortcut
        style{color::from_rgb(106, 106, 106), color::from_rgb(37, 37, 38)},
        // status_bar
        style{color::from_rgb(255, 255, 255), color::from_rgb(0, 122, 204)},
        // selection_highlight
        style{color::from_rgb(255, 255, 255), color::from_rgb(38, 79, 120)},
        // cursor_line
        style{color::default_color(), color::from_rgb(38, 79, 120)},
    };
}

classic_app_theme classic_app_theme::light() {
    return {
        // menu_bar_bg
        style{color::from_rgb(51, 51, 51), color::from_rgb(221, 221, 221)},
        // menu_bar_active
        style{color::from_rgb(255, 255, 255), color::from_rgb(0, 122, 204)},
        // dropdown_border
        style{color::from_rgb(188, 188, 188), color::from_rgb(243, 243, 243)},
        // dropdown_item
        style{color::from_rgb(51, 51, 51), color::from_rgb(243, 243, 243)},
        // dropdown_highlight
        style{color::from_rgb(255, 255, 255), color::from_rgb(0, 122, 204)},
        // dropdown_shortcut
        style{color::from_rgb(128, 128, 128), color::from_rgb(243, 243, 243)},
        // status_bar
        style{color::from_rgb(255, 255, 255), color::from_rgb(0, 122, 204)},
        // selection_highlight
        style{color::from_rgb(0, 0, 0), color::from_rgb(173, 214, 255)},
        // cursor_line
        style{color::default_color(), color::from_rgb(255, 255, 224)},
    };
}

// ═══════════════════════════════════════════════════════════════════════
//  ANSI helpers
// ═══════════════════════════════════════════════════════════════════════

static void ansi_fg_bg(std::string& buf, const style& s) {
    if (s.fg.kind == color_kind::rgb) {
        char seq[24];
        std::snprintf(seq, sizeof(seq), "\x1b[38;2;%d;%d;%dm", s.fg.r, s.fg.g, s.fg.b);
        buf += seq;
    }
    if (s.bg.kind == color_kind::rgb) {
        char seq[24];
        std::snprintf(seq, sizeof(seq), "\x1b[48;2;%d;%d;%dm", s.bg.r, s.bg.g, s.bg.b);
        buf += seq;
    }
}

static const char* k_reset = "\x1b[0m";
static const char* k_tl = "\xe2\x95\xad";
static const char* k_tr = "\xe2\x95\xae";
static const char* k_bl = "\xe2\x95\xb0";
static const char* k_br = "\xe2\x95\xaf";
static const char* k_hz = "\xe2\x94\x80";
static const char* k_vt = "\xe2\x94\x82";
static const char* k_tee_r = "\xe2\x94\x9c";
static const char* k_tee_l = "\xe2\x94\xa4";

// ═══════════════════════════════════════════════════════════════════════
//  impl
// ═══════════════════════════════════════════════════════════════════════

enum class redraw_hint : uint8_t { none, dropdown_only, full };

struct classic_app::impl {
    // Config
    config                cfg;
    int                   num_menus = 0;

    // Callbacks
    content_builder_fn    content_fn;
    status_builder_fn     status_fn;
    overlay_fn            overlay_fn_;
    menu_action_handler   menu_action_fn;
    std::function<bool(const key_event&)> key_fn;

    // Menu state
    int                   active_menu = -1;
    bool                  menu_open   = false;
    menu_tree             mtree;
    menu_state            mstate;
    std::vector<int>      menu_label_x;
    std::vector<int>      menu_label_w;

    // Content scroll
    int  scroll_y      = 0;
    int  content_lines = 0;
    int  viewport_h    = 20;

    // Status bar info
    std::string status_msg = "Ready";
    int  cursor_line = 1;
    int  cursor_col  = 1;

    // Text selection (document coordinates)
    bool selecting       = false;
    int  sel_anchor_line = -1;
    int  sel_anchor_col  = 0;
    int  sel_end_line    = -1;
    int  sel_end_col     = 0;

    // Terminal size
    uint32_t term_w = 80;
    uint32_t term_h = 24;

    // Document lines (for selection rendering)
    std::vector<std::string> doc_lines;

    // Lifecycle
    bool quit_requested = false;

    // ── Selection helpers ──────────────────────────────────────────────

    bool has_selection() const {
        return sel_anchor_line >= 0 && sel_end_line >= 0 &&
               (sel_anchor_line != sel_end_line || sel_anchor_col != sel_end_col);
    }

    void clear_selection() {
        selecting = false;
        sel_anchor_line = sel_end_line = -1;
        sel_anchor_col = sel_end_col = 0;
    }

    void sel_ordered(int& sl, int& sc, int& el, int& ec) const {
        if (sel_anchor_line < sel_end_line ||
            (sel_anchor_line == sel_end_line && sel_anchor_col <= sel_end_col)) {
            sl = sel_anchor_line; sc = sel_anchor_col;
            el = sel_end_line;    ec = sel_end_col;
        } else {
            sl = sel_end_line;    sc = sel_end_col;
            el = sel_anchor_line; ec = sel_anchor_col;
        }
    }

    // ── Menu helpers ──────────────────────────────────────────────────

    void init_label_positions() {
        menu_label_x.resize(static_cast<size_t>(num_menus));
        menu_label_w.resize(static_cast<size_t>(num_menus));
        int x = 1;
        for (int i = 0; i < num_menus; ++i) {
            menu_label_x[static_cast<size_t>(i)] = x;
            int w = static_cast<int>(cfg.menus[static_cast<size_t>(i)].label.size()) + 2;
            menu_label_w[static_cast<size_t>(i)] = w;
            x += w;
        }
    }

    void open_menu(int idx) {
        if (idx < 0 || idx >= num_menus) return;
        active_menu = idx;
        menu_open = true;
        // Build menu_tree from the menu_bar_entry items
        mtree = menu_tree{};
        const auto& items = cfg.menus[static_cast<size_t>(idx)].items;
        for (const auto& item : items) {
            mtree.add_item(item);
        }
        mstate.close_all();
    }

    void close_menu() {
        menu_open = false;
        active_menu = -1;
        mstate.close_all();
    }

    void switch_menu(int dir) {
        if (!menu_open) return;
        int next = (active_menu + dir + num_menus) % num_menus;
        open_menu(next);
    }

    int hit_test_menu_bar(int x, int y) const {
        if (y != 0) return -1;
        for (int i = 0; i < num_menus; ++i) {
            if (x >= menu_label_x[static_cast<size_t>(i)] &&
                x < menu_label_x[static_cast<size_t>(i)] + menu_label_w[static_cast<size_t>(i)]) {
                return i;
            }
        }
        return -1;
    }

    int dropdown_x() const {
        if (active_menu < 0) return 0;
        return menu_label_x[static_cast<size_t>(active_menu)];
    }

    // ── Menu geometry ─────────────────────────────────────────────────

    static int compute_menu_content_w(const std::vector<menu_tree::node>& items) {
        int max_cw = 0;
        for (const auto& item : items) {
            if (item.separator) continue;
            int lw = static_cast<int>(item.label.size());
            int sw = item.shortcut.empty() ? 0 : static_cast<int>(item.shortcut.size());
            int rw = lw + (sw > 0 ? 2 + sw : 0);
            if (item.has_submenu()) rw += 2;
            if (rw > max_cw) max_cw = rw;
        }
        return max_cw + 2;
    }

    // ── Multi-level hit-test ──────────────────────────────────────────

    struct menu_hit {
        int depth = -1;
        int item_index = -1;
    };

    menu_hit hit_test_all_panels(int mx, int my) const {
        if (!menu_open || active_menu < 0) return {};

        const auto& roots = mtree.roots();
        if (roots.empty()) return {};

        const auto& path = mstate.open_path();

        struct panel_rect { int dx, dy, w, h; int item_count; };
        std::vector<panel_rect> panels;

        int dx = dropdown_x();
        int dy = 1;
        int cw = compute_menu_content_w(roots);
        int pw = cw + 2;
        int ph = static_cast<int>(roots.size()) + 2;
        panels.push_back({dx, dy, pw, ph, static_cast<int>(roots.size())});

        const std::vector<menu_tree::node>* current_items = &roots;
        int sub_dx = dx + pw;
        for (int d = 0; d + 1 < static_cast<int>(path.size()); ++d) {
            int parent_idx = path[static_cast<size_t>(d)];
            if (parent_idx < 0 || parent_idx >= static_cast<int>(current_items->size())) break;
            const auto& parent = (*current_items)[static_cast<size_t>(parent_idx)];
            if (!parent.has_submenu()) break;

            int sub_dy = dy + 1 + parent_idx;
            int sub_cw = compute_menu_content_w(parent.children);
            int sub_pw = sub_cw + 2;
            int sub_ph = static_cast<int>(parent.children.size()) + 2;
            panels.push_back({sub_dx, sub_dy, sub_pw, sub_ph,
                              static_cast<int>(parent.children.size())});

            current_items = &parent.children;
            sub_dx += sub_pw;
        }

        for (int d = static_cast<int>(panels.size()) - 1; d >= 0; --d) {
            const auto& p = panels[static_cast<size_t>(d)];
            if (mx > p.dx && mx < p.dx + p.w - 1 &&
                my > p.dy && my < p.dy + p.h - 1) {
                return {d, my - p.dy - 1};
            }
        }
        return {};
    }

    // ── ANSI menu panel rendering ─────────────────────────────────────

    int render_menu_panel(std::string& buf,
                          const std::vector<menu_tree::node>& items,
                          int dx, int dy, int cursor_val) const {
        const auto& theme = cfg.theme;
        int content_w = compute_menu_content_w(items);
        int menu_w = content_w + 2;

        auto move_to = [&](int row, int col) {
            char seq[24];
            std::snprintf(seq, sizeof(seq), "\x1b[%d;%dH", row + 1, col + 1);
            buf += seq;
        };
        auto fill_hz = [&](int count) {
            for (int i = 0; i < count; ++i) buf += k_hz;
        };

        // Top border
        move_to(dy, dx);
        ansi_fg_bg(buf, theme.dropdown_border);
        buf += k_tl;
        fill_hz(content_w);
        buf += k_tr;
        buf += k_reset;

        // Items
        int row = dy + 1;
        for (int idx = 0; idx < static_cast<int>(items.size()); ++idx) {
            const auto& item = items[static_cast<size_t>(idx)];
            move_to(row, dx);

            if (item.separator) {
                ansi_fg_bg(buf, theme.dropdown_border);
                buf += k_tee_r;
                fill_hz(content_w);
                buf += k_tee_l;
                buf += k_reset;
            } else {
                bool selected = (idx == cursor_val);
                const style& fg_style = selected ? theme.dropdown_highlight : theme.dropdown_item;

                ansi_fg_bg(buf, theme.dropdown_border);
                buf += k_vt;
                ansi_fg_bg(buf, fg_style);

                std::string left_part = " " + item.label;
                int left_cols = 1 + static_cast<int>(item.label.size());

                std::string right_part;
                int right_cols = 0;
                if (!item.shortcut.empty()) {
                    right_part = item.shortcut + " ";
                    right_cols = static_cast<int>(item.shortcut.size()) + 1;
                }
                if (item.has_submenu()) {
                    right_part = "\xe2\x96\xb8 ";  // ▸
                    right_cols = 2;
                }

                int gap = content_w - left_cols - right_cols;
                if (gap < 1) gap = 1;

                buf += left_part;
                for (int g = 0; g < gap; ++g) buf += ' ';

                if (!item.shortcut.empty() && !selected) {
                    ansi_fg_bg(buf, theme.dropdown_shortcut);
                    buf += right_part;
                } else {
                    buf += right_part;
                }

                buf += k_reset;
                ansi_fg_bg(buf, theme.dropdown_border);
                buf += k_vt;
                buf += k_reset;
            }
            row++;
        }

        // Bottom border
        move_to(row, dx);
        ansi_fg_bg(buf, theme.dropdown_border);
        buf += k_bl;
        fill_hz(content_w);
        buf += k_br;
        buf += k_reset;

        return menu_w;
    }

    void render_dropdown_ansi(console& con) const {
        if (!menu_open || active_menu < 0) return;

        const auto& roots = mtree.roots();
        if (roots.empty()) return;

        std::string buf;
        buf.reserve(2048);

        int dx = dropdown_x();
        int dy = 1;
        int cursor0 = mstate.cursor_at(0);
        int panel_w = render_menu_panel(buf, roots, dx, dy, cursor0);

        const auto& path = mstate.open_path();
        const std::vector<menu_tree::node>* current_items = &roots;
        int sub_dx = dx + panel_w;
        int sub_dy = dy;

        for (int d = 0; d + 1 < static_cast<int>(path.size()); ++d) {
            int parent_idx = path[static_cast<size_t>(d)];
            if (parent_idx < 0 || parent_idx >= static_cast<int>(current_items->size()))
                break;
            const auto& parent = (*current_items)[static_cast<size_t>(parent_idx)];
            if (!parent.has_submenu()) break;

            sub_dy = dy + 1 + parent_idx;

            int sub_cursor = (d + 1 < static_cast<int>(path.size()))
                             ? mstate.cursor_at(d + 1) : -1;
            int sw = render_menu_panel(buf, parent.children, sub_dx, sub_dy, sub_cursor);

            current_items = &parent.children;
            sub_dx += sw;
        }

        con.write(buf);
    }

    // ── Selection rendering ───────────────────────────────────────────

    void render_selection_ansi(console& con,
                               const std::vector<std::string>& doc_lines) const {
        if (!has_selection()) return;

        int sl, sc, el, ec;
        sel_ordered(sl, sc, el, ec);

        int vis_start = scroll_y;
        int vis_end   = scroll_y + viewport_h;
        if (vis_end > static_cast<int>(doc_lines.size()))
            vis_end = static_cast<int>(doc_lines.size());

        std::string buf;
        buf.reserve(1024);

        auto strip_markup = [](const std::string& s) -> std::string {
            std::string out;
            out.reserve(s.size());
            bool in_tag = false;
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i] == '[') { in_tag = true; continue; }
                if (s[i] == ']') { in_tag = false; continue; }
                if (!in_tag) out += s[i];
            }
            return out;
        };

        static constexpr int k_gutter_cols = 7;

        for (int line = sl; line <= el; ++line) {
            if (line < vis_start || line >= vis_end) continue;

            int screen_row = (line - scroll_y) + 1;

            std::string plain = (line < static_cast<int>(doc_lines.size()))
                                ? strip_markup(doc_lines[static_cast<size_t>(line)]) : "";
            int line_len = static_cast<int>(plain.size());

            int col_start, col_end;
            if (sl == el) {
                col_start = sc;
                col_end   = ec;
            } else if (line == sl) {
                col_start = sc;
                col_end   = line_len;
            } else if (line == el) {
                col_start = 0;
                col_end   = ec;
            } else {
                col_start = 0;
                col_end   = line_len;
            }

            if (col_start > line_len) col_start = line_len;
            if (col_end > line_len) col_end = line_len;
            if (col_start >= col_end && line != el) {
                col_end = col_start + 1;
            }
            if (col_start >= col_end) continue;

            int sx = k_gutter_cols + col_start;

            char seq[32];
            std::snprintf(seq, sizeof(seq), "\x1b[%d;%dH", screen_row + 1, sx + 1);
            buf += seq;
            ansi_fg_bg(buf, cfg.theme.selection_highlight);

            for (int c = col_start; c < col_end; ++c) {
                if (c < line_len) {
                    buf += plain[static_cast<size_t>(c)];
                } else {
                    buf += ' ';
                }
            }
            buf += k_reset;
        }

        if (!buf.empty()) con.write(buf);
    }

    // ── Screen-to-document coordinate conversion ──────────────────────

    static constexpr int k_gutter_cols = 7;

    bool screen_to_doc(int sx, int sy, int& doc_line, int& doc_col) const {
        if (sy < 1 || sy > viewport_h) return false;
        doc_line = scroll_y + (sy - 1);
        if (doc_line >= content_lines) return false;
        doc_col = sx - k_gutter_cols;
        if (doc_col < 0) doc_col = 0;
        return true;
    }

    // ── Find label by global_index ────────────────────────────────────

    static std::string find_label_by_index(const std::vector<menu_tree::node>& nodes, int idx) {
        for (const auto& n : nodes) {
            if (n.global_index == idx) return n.label;
            auto s = find_label_by_index(n.children, idx);
            if (!s.empty()) return s;
        }
        return "";
    }

    // ── Input handling ────────────────────────────────────────────────

    redraw_hint handle_mouse_press(uint32_t mx, uint32_t my) {
        int imx = static_cast<int>(mx);
        int imy = static_cast<int>(my);

        // Check menu bar
        int bar_hit = hit_test_menu_bar(imx, imy);
        if (bar_hit >= 0) {
            if (menu_open && bar_hit == active_menu) {
                close_menu();
            } else {
                open_menu(bar_hit);
            }
            return redraw_hint::full;
        }

        // Check dropdown items
        if (menu_open) {
            auto hit = hit_test_all_panels(imx, imy);
            if (hit.depth >= 0) {
                mstate.click(hit.depth, hit.item_index, mtree);
                if (mstate.was_selected()) {
                    int sel = mstate.selected_item();
                    std::string label = find_label_by_index(mtree.roots(), sel);
                    if (!label.empty() && menu_action_fn) {
                        menu_action_fn(active_menu, sel, label);
                    }
                    mstate.clear_selection();
                    close_menu();
                }
                return redraw_hint::full;
            }
            close_menu();
        }

        // Click in content area — start selection
        {
            int dl, dc;
            if (screen_to_doc(imx, imy, dl, dc)) {
                cursor_line = imy;
                cursor_col = imx + 1;
                selecting = true;
                sel_anchor_line = dl;
                sel_anchor_col  = dc;
                sel_end_line    = dl;
                sel_end_col     = dc;
            } else {
                clear_selection();
            }
        }
        return redraw_hint::full;
    }

    redraw_hint handle_mouse_move(uint32_t mx, uint32_t my) {
        int imx = static_cast<int>(mx);
        int imy = static_cast<int>(my);

        if (menu_open) {
            int bar_hit = hit_test_menu_bar(imx, imy);
            if (bar_hit >= 0 && bar_hit != active_menu) {
                open_menu(bar_hit);
                return redraw_hint::full;
            }

            auto hit = hit_test_all_panels(imx, imy);
            if (hit.depth >= 0) {
                int old_depth = mstate.depth();
                auto old_path = mstate.open_path();

                mstate.hover(hit.depth, hit.item_index, imx, imy, mtree);

                // Auto-open submenu on hover
                const std::vector<menu_tree::node>* items = &mtree.roots();
                for (int d = 0; d < hit.depth; ++d) {
                    int idx = mstate.cursor_at(d);
                    if (idx >= 0 && idx < static_cast<int>(items->size()) &&
                        (*items)[static_cast<size_t>(idx)].has_submenu()) {
                        items = &(*items)[static_cast<size_t>(idx)].children;
                    } else break;
                }
                if (hit.item_index >= 0 &&
                    hit.item_index < static_cast<int>(items->size()) &&
                    (*items)[static_cast<size_t>(hit.item_index)].has_submenu()) {
                    if (mstate.depth() == hit.depth) {
                        mstate.open_submenu(mtree);
                    }
                }

                if (mstate.depth() != old_depth ||
                    mstate.open_path() != old_path) {
                    return redraw_hint::full;
                }
                return redraw_hint::dropdown_only;
            }
        }
        return redraw_hint::none;
    }

    redraw_hint handle_mouse_drag(uint32_t mx, uint32_t my) {
        if (selecting) {
            int dl, dc;
            if (screen_to_doc(static_cast<int>(mx), static_cast<int>(my), dl, dc)) {
                int old_el = sel_end_line, old_ec = sel_end_col;
                sel_end_line = dl;
                sel_end_col  = dc;
                cursor_line = static_cast<int>(my);
                cursor_col = static_cast<int>(mx) + 1;
                if (dl != old_el || dc != old_ec)
                    return redraw_hint::full;
            }
        }
        return redraw_hint::none;
    }

    redraw_hint handle_mouse_release(uint32_t mx, uint32_t my) {
        if (selecting) {
            selecting = false;
            int dl, dc;
            if (screen_to_doc(static_cast<int>(mx), static_cast<int>(my), dl, dc)) {
                sel_end_line = dl;
                sel_end_col  = dc;
            }
            if (!has_selection()) {
                clear_selection();
            }
            return redraw_hint::full;
        }
        return redraw_hint::none;
    }

    redraw_hint handle_key_menu_open(const key_event& ke) {
        if (ke.key == special_key::up) {
            mstate.move_up(mtree);
            return redraw_hint::dropdown_only;
        }
        if (ke.key == special_key::down) {
            mstate.move_down(mtree);
            return redraw_hint::dropdown_only;
        }
        if (ke.key == special_key::right) {
            if (mstate.depth() == 0) {
                const auto& items = mtree.roots();
                int cur = mstate.cursor();
                if (cur >= 0 && cur < static_cast<int>(items.size()) &&
                    items[static_cast<size_t>(cur)].has_submenu()) {
                    mstate.open_submenu(mtree);
                    return redraw_hint::dropdown_only;
                } else {
                    switch_menu(1);
                    return redraw_hint::full;
                }
            } else {
                mstate.open_submenu(mtree);
                return redraw_hint::dropdown_only;
            }
        }
        if (ke.key == special_key::left) {
            if (mstate.depth() > 0) {
                mstate.close_submenu();
                return redraw_hint::full;
            } else {
                switch_menu(-1);
                return redraw_hint::full;
            }
        }
        if (ke.key == special_key::enter) {
            mstate.select(mtree);
            if (mstate.was_selected()) {
                int sel = mstate.selected_item();
                std::string label = find_label_by_index(mtree.roots(), sel);
                if (!label.empty() && menu_action_fn) {
                    menu_action_fn(active_menu, sel, label);
                }
                mstate.clear_selection();
                close_menu();
            }
            return redraw_hint::full;
        }
        if (ke.key == special_key::escape) {
            close_menu();
            return redraw_hint::full;
        }
        return redraw_hint::none;
    }

    redraw_hint handle_key_normal(const key_event& ke) {
        // Let user handler try first
        if (key_fn && key_fn(ke)) return redraw_hint::full;

        int max_scroll = content_lines - viewport_h;
        if (max_scroll < 0) max_scroll = 0;

        if (ke.key == special_key::up) {
            if (scroll_y > 0) --scroll_y;
            return redraw_hint::full;
        }
        if (ke.key == special_key::down) {
            if (scroll_y < max_scroll) ++scroll_y;
            return redraw_hint::full;
        }
        if (ke.key == special_key::page_up) {
            scroll_y -= viewport_h;
            if (scroll_y < 0) scroll_y = 0;
            return redraw_hint::full;
        }
        if (ke.key == special_key::page_down) {
            scroll_y += viewport_h;
            if (scroll_y > max_scroll) scroll_y = max_scroll;
            return redraw_hint::full;
        }
        if (ke.key == special_key::home) {
            scroll_y = 0;
            return redraw_hint::full;
        }
        if (ke.key == special_key::end) {
            scroll_y = max_scroll;
            return redraw_hint::full;
        }
        if (ke.key == special_key::escape) {
            if (has_selection()) {
                clear_selection();
                return redraw_hint::full;
            }
            quit_requested = true;
            return redraw_hint::none;
        }
        // 'q' / 'Q' to quit
        if (ke.codepoint == 'q' || ke.codepoint == 'Q') {
            quit_requested = true;
            return redraw_hint::none;
        }
        return redraw_hint::none;
    }

    redraw_hint handle_input(const input_event& ev) {
        mstate.tick(std::chrono::milliseconds(cfg.poll_interval_ms));

        if (auto* me = std::get_if<mouse_event>(&ev)) {
            switch (me->action) {
            case mouse_action::press:
                if (me->button == mouse_button::left)
                    return handle_mouse_press(me->x, me->y);
                break;
            case mouse_action::move:
                return handle_mouse_move(me->x, me->y);
            case mouse_action::drag:
                return handle_mouse_drag(me->x, me->y);
            case mouse_action::release:
                return handle_mouse_release(me->x, me->y);
            }
            return redraw_hint::none;
        }

        if (auto* ke = std::get_if<key_event>(&ev)) {
            if (menu_open) return handle_key_menu_open(*ke);
            return handle_key_normal(*ke);
        }

        if (auto* re = std::get_if<resize_event>(&ev)) {
            if (re->width > 0)  term_w = re->width;
            if (re->height > 0) {
                term_h = re->height;
                viewport_h = static_cast<int>(term_h) - 3;
            }
            return redraw_hint::full;
        }

        return redraw_hint::none;
    }

    // ── View builders ─────────────────────────────────────────────────

    auto build_menu_bar_row() const {
        std::string bar;
        bar += " ";
        for (int i = 0; i < num_menus; ++i) {
            const auto& label = cfg.menus[static_cast<size_t>(i)].label;
            if (menu_open && i == active_menu) {
                bar += "[bright_white on_rgb("
                    + std::to_string(cfg.theme.menu_bar_active.bg.r) + ","
                    + std::to_string(cfg.theme.menu_bar_active.bg.g) + ","
                    + std::to_string(cfg.theme.menu_bar_active.bg.b) + ")] "
                    + label + " [/]";
            } else {
                bar += "[rgb("
                    + std::to_string(cfg.theme.menu_bar_bg.fg.r) + ","
                    + std::to_string(cfg.theme.menu_bar_bg.fg.g) + ","
                    + std::to_string(cfg.theme.menu_bar_bg.fg.b) + ") on_rgb("
                    + std::to_string(cfg.theme.menu_bar_bg.bg.r) + ","
                    + std::to_string(cfg.theme.menu_bar_bg.bg.g) + ","
                    + std::to_string(cfg.theme.menu_bar_bg.bg.b) + ")] "
                    + label + " [/]";
            }
        }
        auto tb = text_builder(bar);
        tb.style_override(cfg.theme.menu_bar_bg);
        return tb;
    }

    // ── Render frame ──────────────────────────────────────────────────

    class base_view {
    public:
        base_view(const impl& self, const std::vector<std::string>& doc)
            : self_(self), doc_(doc) {}

        node_id flatten_into(detail::scene& s) const {
            rows_builder base;
            base.gap(0);
            base.add(self_.build_menu_bar_row());

            // Content
            rows_builder content;
            content.gap(0);
            if (self_.content_fn) {
                self_.content_fn(content, self_.scroll_y, self_.viewport_h);
            }
            content.key("doc-content");
            base.add(std::move(content), 1);

            // Status bar
            status_bar_builder sb;
            if (self_.status_fn) {
                self_.status_fn(sb);
            }
            sb.style_override(self_.cfg.theme.status_bar);
            sb.key("status-bar");
            base.add(std::move(sb));

            base.key("app-base");
            return base.flatten_into(s);
        }

    private:
        const impl& self_;
        const std::vector<std::string>& doc_;
    };

    void render_frame(console& con, const std::vector<std::string>& doc,
                      bool dropdown_only = false) const {
        con.write("\x1b[?2026h");

        if (!dropdown_only) {
            con.write("\x1b[H");
            con.print_widget(base_view(*this, doc), term_w, term_h);
            render_selection_ansi(con, doc);
            if (overlay_fn_) {
                std::string overlay_buf;
                overlay_fn_(overlay_buf, term_w, term_h, scroll_y, viewport_h);
                if (!overlay_buf.empty()) con.write(overlay_buf);
            }
        }
        render_dropdown_ansi(con);

        con.write("\x1b[?2026l");
        std::fflush(stdout);
    }
};

// ═══════════════════════════════════════════════════════════════════════
//  classic_app public API
// ═══════════════════════════════════════════════════════════════════════

classic_app::classic_app(config cfg)
    : impl_(std::make_unique<impl>())
{
    impl_->cfg = std::move(cfg);
    impl_->num_menus = static_cast<int>(impl_->cfg.menus.size());
    impl_->init_label_positions();
    impl_->status_msg = "Ready";
}

classic_app::~classic_app() = default;
classic_app::classic_app(classic_app&&) noexcept = default;
classic_app& classic_app::operator=(classic_app&&) noexcept = default;

void classic_app::set_content(content_builder_fn fn) { impl_->content_fn = std::move(fn); }
void classic_app::set_status(status_builder_fn fn)   { impl_->status_fn = std::move(fn); }
void classic_app::set_overlay(overlay_fn fn)          { impl_->overlay_fn_ = std::move(fn); }
void classic_app::on_menu_action(menu_action_handler fn) { impl_->menu_action_fn = std::move(fn); }
void classic_app::on_key(std::function<bool(const key_event&)> fn) { impl_->key_fn = std::move(fn); }

void classic_app::quit() { impl_->quit_requested = true; }

int      classic_app::scroll_y()    const { return impl_->scroll_y; }
int      classic_app::viewport_h()  const { return impl_->viewport_h; }
int      classic_app::cursor_line() const { return impl_->cursor_line; }
int      classic_app::cursor_col()  const { return impl_->cursor_col; }
uint32_t classic_app::term_width()  const { return impl_->term_w; }
uint32_t classic_app::term_height() const { return impl_->term_h; }

bool classic_app::has_selection() const { return impl_->has_selection(); }

void classic_app::selection_range(int& sl, int& sc, int& el, int& ec) const {
    impl_->sel_ordered(sl, sc, el, ec);
}

void classic_app::scroll_to(int line) {
    impl_->scroll_y = line;
    int max_scroll = impl_->content_lines - impl_->viewport_h;
    if (max_scroll < 0) max_scroll = 0;
    if (impl_->scroll_y < 0) impl_->scroll_y = 0;
    if (impl_->scroll_y > max_scroll) impl_->scroll_y = max_scroll;
}

void classic_app::set_status_message(const std::string& msg) {
    impl_->status_msg = msg;
}

void classic_app::set_content_lines(int total) {
    impl_->content_lines = total;
}

void classic_app::set_document_lines(const std::vector<std::string>& lines) {
    impl_->doc_lines = lines;
}

void classic_app::run() {
    if (!terminal::is_tty()) {
        std::fprintf(stderr, "classic_app requires a terminal (TTY).\n");
        return;
    }

    console con;

    const auto& doc_lines = impl_->doc_lines;

    // Alternate screen + hide cursor
    con.write("\x1b[?1049h");
    con.write("\x1b[?25l");

    raw_mode raw;

    // Get terminal size
    auto sz = terminal::get_size();
    impl_->term_w = sz.width > 0 ? sz.width : 80;
    impl_->term_h = sz.height > 0 ? sz.height : 24;
    impl_->viewport_h = static_cast<int>(impl_->term_h) - 3;

    // Initial render
    impl_->render_frame(con, doc_lines);

    while (!impl_->quit_requested) {
        auto ev = poll_event(impl_->cfg.poll_interval_ms);

        redraw_hint hint = redraw_hint::none;

        if (ev) {
            hint = impl_->handle_input(*ev);
        } else {
            // Tick menu timers even on timeout
            impl_->mstate.tick(std::chrono::milliseconds(impl_->cfg.poll_interval_ms));
        }

        // Re-detect terminal size (for platforms without resize events)
        auto new_sz = terminal::get_size();
        if (new_sz.width > 0 && new_sz.width != impl_->term_w) {
            impl_->term_w = new_sz.width;
            hint = redraw_hint::full;
        }
        if (new_sz.height > 0 && new_sz.height != impl_->term_h) {
            impl_->term_h = new_sz.height;
            impl_->viewport_h = static_cast<int>(impl_->term_h) - 3;
            hint = redraw_hint::full;
        }

        if (hint == redraw_hint::full) {
            impl_->render_frame(con, doc_lines);
        } else if (hint == redraw_hint::dropdown_only) {
            impl_->render_frame(con, doc_lines, true);
        }
    }

    // Restore screen + show cursor
    con.write("\x1b[?25h");
    con.write("\x1b[?1049l");
}

}  // namespace tapiru
