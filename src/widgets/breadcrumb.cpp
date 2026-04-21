/**
 * @file breadcrumb.cpp
 * @brief breadcrumb_builder flatten_into implementation.
 */

#include "tapiru/widgets/breadcrumb.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

    breadcrumb_builder &breadcrumb_builder::key(std::string_view k) {
        key_ = detail::fnv1a_hash(k);
        return *this;
    }

    node_id breadcrumb_builder::flatten_into(detail::scene &s) const {
        int act = active_ ? *active_ : static_cast<int>(items_.size()) - 1;

        auto cols = columns_builder();
        for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
            if (i > 0) {
                auto sep_tb = text_builder(separator_);
                sep_tb.style_override(sep_sty_);
                cols.add(std::move(sep_tb));
            }
            auto item_tb = text_builder(items_[static_cast<size_t>(i)]);
            if (i == act) {
                item_tb.style_override(active_sty_);
            } else {
                item_tb.style_override(item_sty_);
            }
            cols.add(std::move(item_tb));
        }

        auto id = cols.flatten_into(s);
        if (z_order_ != 0) {
            s.set_z_order(id, z_order_);
        }
        return id;
    }

} // namespace tapiru
