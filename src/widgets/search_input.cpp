/**
 * @file search_input.cpp
 * @brief search_input_builder flatten_into implementation.
 */

#include "tapiru/widgets/search_input.h"
#include "tapiru/widgets/builders.h"
#include "detail/scene.h"

namespace tapiru {

search_input_builder& search_input_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id search_input_builder::flatten_into(detail::scene& s) const {
    std::string query_text = query_ ? *query_ : "";
    std::string display = query_text.empty() ? placeholder_ : query_text;

    // Build match info
    std::string match_info;
    if (match_count_) {
        int total = *match_count_;
        int cur = current_match_ ? *current_match_ : 0;
        if (total > 0) {
            match_info = " " + std::to_string(cur + 1) + "/" + std::to_string(total);
        } else if (!query_text.empty()) {
            match_info = " No matches";
        }
    }

    // Compose: columns([icon] [input] [match_info])
    auto cols = columns_builder();

    auto icon_tb = text_builder("\xf0\x9f\x94\x8d ");  // 🔍
    cols.add(std::move(icon_tb));

    auto input_tb = text_builder(display);
    input_tb.style_override(input_sty_);
    cols.add(std::move(input_tb));

    if (!match_info.empty()) {
        auto match_tb = text_builder(match_info);
        match_tb.style_override(match_sty_);
        cols.add(std::move(match_tb));
    }

    if (border_ != border_style::none) {
        auto panel = panel_builder(std::move(cols));
        panel.border(border_);
        auto id = panel.flatten_into(s);
        if (z_order_ != 0) s.set_z_order(id, z_order_);
        return id;
    }

    auto id = cols.flatten_into(s);
    if (z_order_ != 0) s.set_z_order(id, z_order_);
    return id;
}

}  // namespace tapiru
