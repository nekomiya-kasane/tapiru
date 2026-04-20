#pragma once

/**
 * @file search_input.h
 * @brief Search input widget — text input with match count and navigation.
 */

#include <string>
#include <string_view>

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

class TAPIRU_API search_input_builder {
public:
    search_input_builder() = default;

    search_input_builder& query(const std::string* q)        { query_ = q; return *this; }
    search_input_builder& match_count(const int* c)          { match_count_ = c; return *this; }
    search_input_builder& current_match(const int* c)        { current_match_ = c; return *this; }
    search_input_builder& placeholder(std::string_view p)    { placeholder_ = p; return *this; }
    search_input_builder& width(uint32_t w)                  { width_ = w; return *this; }
    search_input_builder& input_style(const style& s)        { input_sty_ = s; return *this; }
    search_input_builder& match_style(const style& s)        { match_sty_ = s; return *this; }
    search_input_builder& border(border_style bs)            { border_ = bs; return *this; }
    search_input_builder& z_order(int16_t z)                 { z_order_ = z; return *this; }
    search_input_builder& key(std::string_view k);

    node_id flatten_into(detail::scene& s) const;

private:
    const std::string* query_        = nullptr;
    const int*         match_count_  = nullptr;
    const int*         current_match_ = nullptr;
    std::string        placeholder_  = "Search...";
    uint32_t           width_        = 20;
    style              input_sty_;
    style              match_sty_;
    border_style       border_       = border_style::rounded;
    int16_t            z_order_      = 0;
    uint64_t           key_          = 0;
};

}  // namespace tapiru
