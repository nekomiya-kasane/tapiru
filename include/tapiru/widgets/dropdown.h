#pragma once

/**
 * @file dropdown.h
 * @brief Dropdown select widget — a button that opens a selectable list.
 */

#include <string>
#include <string_view>
#include <vector>

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

class TAPIRU_API dropdown_builder {
public:
    dropdown_builder() = default;

    dropdown_builder& add_option(std::string_view label) {
        options_.emplace_back(label);
        return *this;
    }

    dropdown_builder& selected(const int* idx)           { selected_ = idx; return *this; }
    dropdown_builder& open(const bool* o)                { open_ = o; return *this; }
    dropdown_builder& placeholder(std::string_view p)    { placeholder_ = p; return *this; }
    dropdown_builder& button_style(const style& s)       { button_sty_ = s; return *this; }
    dropdown_builder& highlight_style(const style& s)    { highlight_sty_ = s; return *this; }
    dropdown_builder& item_style(const style& s)         { item_sty_ = s; return *this; }
    dropdown_builder& border(border_style bs)            { border_ = bs; return *this; }
    dropdown_builder& width(uint32_t w)                  { width_ = w; return *this; }
    dropdown_builder& z_order(int16_t z)                 { z_order_ = z; return *this; }
    dropdown_builder& key(std::string_view k);

    node_id flatten_into(detail::scene& s) const;

private:
    std::vector<std::string> options_;
    const int*   selected_    = nullptr;
    const bool*  open_        = nullptr;
    std::string  placeholder_ = "Select...";
    style        button_sty_;
    style        highlight_sty_;
    style        item_sty_;
    border_style border_      = border_style::rounded;
    uint32_t     width_       = 0;
    int16_t      z_order_     = 0;
    uint64_t     key_         = 0;
};

}  // namespace tapiru
