/**
 * @file virtual_list.cpp
 * @brief Virtual list builder implementation.
 */

#include "tapiru/widgets/virtual_list.h"

#include "detail/scene.h"
#include "detail/widget_types.h"

#include <algorithm>
#include <string>

namespace tapiru {

node_id virtual_list_builder::flatten_into(detail::scene &s) const {
    if (!item_fn_ || total_ == 0 || visible_ == 0) {
        return text_builder("").flatten_into(s);
    }

    uint32_t max_offset = (total_ > visible_) ? total_ - visible_ : 0;
    uint32_t eff_offset = std::min(offset_, max_offset);
    uint32_t end = std::min(eff_offset + visible_, total_);

    // Build visible items using the item_builder callback
    detail::rows_data rd;
    rd.gap = 0;

    auto pi = s.add_rows(std::move(rd));
    auto rows_id = s.add_node(detail::widget_type::rows, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(rows_id, z_order_);

    for (uint32_t i = eff_offset; i < end; ++i) {
        auto tb = item_fn_(i);
        auto child_id = tb.flatten_into(s);
        s.get_rows(pi).children.push_back(child_id);
        s.get_rows(pi).flex_ratios.push_back(0);
    }

    return rows_id;
}

} // namespace tapiru
