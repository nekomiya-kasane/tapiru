/**
 * @file textarea.cpp
 * @brief textarea_builder flatten_into implementation.
 */

#include "tapiru/widgets/textarea.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <vector>

namespace tapiru {

textarea_builder &textarea_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

// ── Per-character fragment builder ──────────────────────────────────────

namespace {

struct styled_range {
    int col_start;
    int col_end;
    style sty;
    int priority; // higher = wins overlap
};

// Build text_fragment vector for a single line, applying styled ranges.
// Ranges may overlap; higher priority wins.
std::vector<text_fragment> build_line_fragments(const std::string &line, const style &base_sty,
                                                const std::vector<styled_range> &ranges) {
    int len = static_cast<int>(line.size());
    if (len == 0 && ranges.empty()) {
        return {{" ", base_sty}};
    }

    // For each column, determine the effective style.
    // We use a simple approach: for short lines this is efficient enough.
    int extent = std::max(len, 1);
    // Extend extent to cover any range that goes past line end (e.g. cursor on empty line)
    for (const auto &r : ranges) {
        if (r.col_end > extent) {
            extent = r.col_end;
        }
    }

    // Build per-column style array
    std::vector<style> col_styles(static_cast<size_t>(extent), base_sty);
    std::vector<int> col_prio(static_cast<size_t>(extent), -1);

    for (const auto &r : ranges) {
        int cs = std::max(r.col_start, 0);
        int ce = std::min(r.col_end, extent);
        for (int c = cs; c < ce; ++c) {
            if (r.priority > col_prio[static_cast<size_t>(c)]) {
                col_prio[static_cast<size_t>(c)] = r.priority;
                col_styles[static_cast<size_t>(c)] = r.sty;
            }
        }
    }

    // Merge consecutive columns with same style into fragments
    std::vector<text_fragment> frags;
    int frag_start = 0;
    for (int c = 1; c <= extent; ++c) {
        bool same = (c < extent) &&
                    col_styles[static_cast<size_t>(c)].fg.kind == col_styles[static_cast<size_t>(frag_start)].fg.kind &&
                    col_styles[static_cast<size_t>(c)].fg.r == col_styles[static_cast<size_t>(frag_start)].fg.r &&
                    col_styles[static_cast<size_t>(c)].fg.g == col_styles[static_cast<size_t>(frag_start)].fg.g &&
                    col_styles[static_cast<size_t>(c)].fg.b == col_styles[static_cast<size_t>(frag_start)].fg.b &&
                    col_styles[static_cast<size_t>(c)].bg.kind == col_styles[static_cast<size_t>(frag_start)].bg.kind &&
                    col_styles[static_cast<size_t>(c)].bg.r == col_styles[static_cast<size_t>(frag_start)].bg.r &&
                    col_styles[static_cast<size_t>(c)].bg.g == col_styles[static_cast<size_t>(frag_start)].bg.g &&
                    col_styles[static_cast<size_t>(c)].bg.b == col_styles[static_cast<size_t>(frag_start)].bg.b &&
                    col_styles[static_cast<size_t>(c)].attrs == col_styles[static_cast<size_t>(frag_start)].attrs;

        if (!same) {
            std::string text;
            for (int k = frag_start; k < c; ++k) {
                if (k < len) {
                    text += line[static_cast<size_t>(k)];
                } else {
                    text += ' ';
                }
            }
            frags.push_back({std::move(text), col_styles[static_cast<size_t>(frag_start)]});
            frag_start = c;
        }
    }

    return frags;
}

// Check if a line is within a selection range, and return column bounds.
bool selection_cols_for_line(const text_range &sel, int line_idx, int line_len, int &col_start, int &col_end) {
    // Normalize selection direction
    int sr = sel.start_row, sc = sel.start_col;
    int er = sel.end_row, ec = sel.end_col;
    if (sr > er || (sr == er && sc > ec)) {
        std::swap(sr, er);
        std::swap(sc, ec);
    }

    if (line_idx < sr || line_idx > er) {
        return false;
    }

    if (sr == er) {
        col_start = sc;
        col_end = ec;
    } else if (line_idx == sr) {
        col_start = sc;
        col_end = line_len > 0 ? line_len : 1;
    } else if (line_idx == er) {
        col_start = 0;
        col_end = ec;
    } else {
        col_start = 0;
        col_end = line_len > 0 ? line_len : 1;
    }
    return col_start < col_end;
}

} // anonymous namespace

// ── flatten_into ────────────────────────────────────────────────────────

node_id textarea_builder::flatten_into(detail::scene &s) const {
    // Split text into lines
    std::vector<std::string> lines;
    if (text_ && !text_->empty()) {
        std::istringstream iss(*text_);
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(std::move(line));
        }
    }
    if (lines.empty()) {
        lines.emplace_back("");
    }

    int scroll = scroll_row_ ? *scroll_row_ : 0;
    if (scroll < 0) {
        scroll = 0;
    }
    int cur_row = cursor_row_ ? *cursor_row_ : 0;
    int cur_col = cursor_col_ ? *cursor_col_ : 0;

    uint32_t visible_h = height_;
    uint32_t gutter_w = 0;
    if (line_numbers_ || relative_nums_) {
        int total = static_cast<int>(lines.size());
        gutter_w = 1;
        while (total >= 10) {
            gutter_w++;
            total /= 10;
        }
        gutter_w += 1; // space after number
    }

    // Build rows of visible lines
    detail::rows_data rd;
    rd.gap = 0;
    auto rows_pi = s.add_rows(std::move(rd));
    auto rows_id = s.add_node(detail::widget_type::rows, rows_pi, detail::no_node, 0);

    for (uint32_t vy = 0; vy < visible_h; ++vy) {
        int line_idx = scroll + static_cast<int>(vy);
        bool is_content = line_idx < static_cast<int>(lines.size());
        bool is_cursor_line = (line_idx == cur_row);

        // ── Gutter ──────────────────────────────────────────────────
        std::string gutter_text;
        if ((line_numbers_ || relative_nums_) && gutter_w > 0) {
            if (is_content) {
                int display_num;
                if (relative_nums_) {
                    display_num = is_cursor_line ? (line_idx + 1) : std::abs(line_idx - cur_row);
                } else {
                    display_num = line_idx + 1;
                }
                std::string num = std::to_string(display_num);
                while (num.size() < gutter_w - 1) {
                    num = " " + num;
                }
                num += " ";
                gutter_text = std::move(num);
            } else {
                gutter_text = std::string(gutter_w, ' ');
            }
        }

        // ── Line content with per-character styling ─────────────────
        if (is_content) {
            const auto &line = lines[static_cast<size_t>(line_idx)];
            int line_len = static_cast<int>(line.size());

            // Base style: cursor line or normal
            style base = is_cursor_line ? cursor_sty_ : text_sty_;

            // Collect styled ranges
            std::vector<styled_range> ranges;

            // Selection (priority 1)
            if (selection_ && selection_->start_row >= 0) {
                int cs, ce;
                if (selection_cols_for_line(*selection_, line_idx, line_len, cs, ce)) {
                    ranges.push_back({cs, ce, selection_sty_, 1});
                }
            }

            // Search matches (priority 2)
            if (matches_) {
                for (const auto &m : *matches_) {
                    if (m.start_row == line_idx) {
                        ranges.push_back({m.start_col, m.end_col, match_sty_, 2});
                    }
                }
            }

            // Block cursor (priority 3 — highest, always visible)
            if (show_cursor_ && is_cursor_line) {
                int cc = cur_col;
                if (cc >= line_len) {
                    cc = line_len; // cursor at EOL
                }
                ranges.push_back({cc, cc + 1, cursor_char_sty_, 3});
            }

            auto content_frags = build_line_fragments(line, base, ranges);

            // Combine gutter + content into a single text node
            detail::text_data td;
            td.overflow = overflow_mode::truncate;
            td.align = justify::left;

            if (!gutter_text.empty()) {
                td.fragments.push_back({std::move(gutter_text), line_num_sty_});
            }
            for (auto &f : content_frags) {
                td.fragments.push_back(std::move(f));
            }

            auto pi = s.add_text(std::move(td));
            auto text_id = s.add_node(detail::widget_type::text, pi, detail::no_node, 0);
            s.get_rows(rows_pi).children.push_back(text_id);
            s.get_rows(rows_pi).flex_ratios.push_back(0);
        } else {
            // EOF line
            detail::text_data td;
            td.overflow = overflow_mode::truncate;
            td.align = justify::left;

            if (!gutter_text.empty()) {
                td.fragments.push_back({std::move(gutter_text), line_num_sty_});
            }
            td.fragments.push_back({"~", eof_sty_.fg.kind != color_kind::default_color ? eof_sty_ : text_sty_});

            auto pi = s.add_text(std::move(td));
            auto text_id = s.add_node(detail::widget_type::text, pi, detail::no_node, 0);
            s.get_rows(rows_pi).children.push_back(text_id);
            s.get_rows(rows_pi).flex_ratios.push_back(0);
        }
    }

    // Wrap in sized_box for fixed dimensions
    detail::sized_box_data sb;
    sb.content = rows_id;
    sb.fixed_w = width_ + gutter_w;
    sb.fixed_h = visible_h;
    auto sb_pi = s.add_sized_box(std::move(sb));
    auto sb_id = s.add_node(detail::widget_type::sized_box, sb_pi, detail::no_node, 0);

    // Wrap in panel if border is set
    if (border_ != border_style::none) {
        detail::panel_data pd;
        pd.border = border_;
        pd.border_sty = border_sty_;
        pd.content = sb_id;
        auto panel_pi = s.add_panel(std::move(pd));
        auto panel_id = s.add_node(detail::widget_type::panel, panel_pi, detail::no_node, key_);
        if (z_order_ != 0) {
            s.set_z_order(panel_id, z_order_);
        }
        return panel_id;
    }

    if (z_order_ != 0) {
        s.set_z_order(sb_id, z_order_);
    }
    return sb_id;
}

} // namespace tapiru
