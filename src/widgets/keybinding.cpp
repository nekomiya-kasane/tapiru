/**
 * @file keybinding.cpp
 * @brief keybinding_builder flatten_into implementation.
 */

#include "tapiru/widgets/keybinding.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

keybinding_builder &keybinding_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id keybinding_builder::flatten_into(detail::scene &s) const {
    // Each entry is a row: [key_badge] [separator] [description]
    detail::rows_data rd;
    rd.gap = 0;
    auto rows_pi = s.add_rows(std::move(rd));
    auto rows_id = s.add_node(detail::widget_type::rows, rows_pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(rows_id, z_order_);

    for (const auto &entry : entries_) {
        auto cols = columns_builder();

        auto key_tb = text_builder("[" + entry.keys + "]");
        key_tb.style_override(key_sty_);
        cols.add(std::move(key_tb));

        auto sep_tb = text_builder(separator_);
        cols.add(std::move(sep_tb));

        auto desc_tb = text_builder(entry.description);
        desc_tb.style_override(desc_sty_);
        cols.add(std::move(desc_tb));

        auto row_id = cols.flatten_into(s);
        s.get_rows(rows_pi).children.push_back(row_id);
        s.get_rows(rows_pi).flex_ratios.push_back(0);
    }

    return rows_id;
}

} // namespace tapiru
