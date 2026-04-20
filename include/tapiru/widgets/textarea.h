#pragma once

/**
 * @file textarea.h
 * @brief Multi-line text area widget with cursor and scroll support.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

namespace detail {
class scene;
}
using node_id = uint32_t;

/** @brief A range of text identified by (start_row,start_col) to (end_row,end_col). */
struct text_range {
    int start_row = -1, start_col = 0;
    int end_row = -1, end_col = 0;
};

class TAPIRU_API textarea_builder {
  public:
    textarea_builder() = default;

    textarea_builder &text(const std::string *t) {
        text_ = t;
        return *this;
    }
    textarea_builder &cursor_row(const int *r) {
        cursor_row_ = r;
        return *this;
    }
    textarea_builder &cursor_col(const int *c) {
        cursor_col_ = c;
        return *this;
    }
    textarea_builder &scroll_row(const int *r) {
        scroll_row_ = r;
        return *this;
    }
    textarea_builder &width(uint32_t w) {
        width_ = w;
        return *this;
    }
    textarea_builder &height(uint32_t h) {
        height_ = h;
        return *this;
    }
    textarea_builder &text_style(const style &s) {
        text_sty_ = s;
        return *this;
    }
    textarea_builder &cursor_style(const style &s) {
        cursor_sty_ = s;
        return *this;
    }
    textarea_builder &line_number_style(const style &s) {
        line_num_sty_ = s;
        return *this;
    }
    textarea_builder &show_line_numbers(bool v = true) {
        line_numbers_ = v;
        return *this;
    }
    textarea_builder &relative_line_numbers(bool v = true) {
        relative_nums_ = v;
        return *this;
    }

    textarea_builder &selection(const text_range *sel) {
        selection_ = sel;
        return *this;
    }
    textarea_builder &selection_style(const style &s) {
        selection_sty_ = s;
        return *this;
    }

    textarea_builder &show_cursor(bool v = true) {
        show_cursor_ = v;
        return *this;
    }
    textarea_builder &cursor_char_style(const style &s) {
        cursor_char_sty_ = s;
        return *this;
    }

    textarea_builder &matches(const std::vector<text_range> *m) {
        matches_ = m;
        return *this;
    }
    textarea_builder &match_style(const style &s) {
        match_sty_ = s;
        return *this;
    }

    textarea_builder &eof_style(const style &s) {
        eof_sty_ = s;
        return *this;
    }

    textarea_builder &border(border_style bs) {
        border_ = bs;
        return *this;
    }
    textarea_builder &border_style_override(const style &s) {
        border_sty_ = s;
        return *this;
    }
    textarea_builder &z_order(int16_t z) {
        z_order_ = z;
        return *this;
    }
    textarea_builder &key(std::string_view k);

    node_id flatten_into(detail::scene &s) const;

  private:
    const std::string *text_ = nullptr;
    const int *cursor_row_ = nullptr;
    const int *cursor_col_ = nullptr;
    const int *scroll_row_ = nullptr;
    uint32_t width_ = 40;
    uint32_t height_ = 10;
    style text_sty_;
    style cursor_sty_;
    style line_num_sty_;
    bool line_numbers_ = false;
    bool relative_nums_ = false;
    const text_range *selection_ = nullptr;
    style selection_sty_;
    bool show_cursor_ = false;
    style cursor_char_sty_;
    const std::vector<text_range> *matches_ = nullptr;
    style match_sty_;
    style eof_sty_;
    border_style border_ = border_style::rounded;
    style border_sty_;
    int16_t z_order_ = 0;
    uint64_t key_ = 0;
};

} // namespace tapiru
