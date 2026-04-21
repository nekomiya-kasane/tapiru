/**
 * @file log_viewer.cpp
 * @brief log_viewer_builder flatten_into implementation.
 */

#include "tapiru/widgets/log_viewer.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

log_viewer_builder &log_viewer_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id log_viewer_builder::flatten_into(detail::scene &s) const {
    // Filter entries
    std::vector<const log_entry *> visible;
    if (entries_) {
        for (const auto &e : *entries_) {
            if (e.level < min_level_) {
                continue;
            }
            if (filter_ && !filter_->empty()) {
                if (e.message.find(*filter_) == std::string::npos) {
                    continue;
                }
            }
            visible.push_back(&e);
        }
    }

    int scroll = scroll_ ? *scroll_ : 0;
    if (scroll < 0) {
        scroll = 0;
    }

    // Build rows of visible log lines
    detail::rows_data rd;
    rd.gap = 0;
    auto rows_pi = s.add_rows(std::move(rd));
    auto rows_id = s.add_node(detail::widget_type::rows, rows_pi, detail::no_node, 0);

    for (uint32_t vy = 0; vy < height_; ++vy) {
        int idx = scroll + static_cast<int>(vy);
        std::string line;

        if (idx < static_cast<int>(visible.size())) {
            const auto &entry = *visible[static_cast<size_t>(idx)];
            if (show_ts_ && !entry.timestamp.empty()) {
                line += "[" + entry.timestamp + "] ";
            }
            if (show_lvl_) {
                line += "[" + std::string(log_level_name(entry.level)) + "] ";
            }
            line += entry.message;
        }

        auto tb = text_builder(line);
        auto text_id = tb.flatten_into(s);
        s.get_rows(rows_pi).children.push_back(text_id);
        s.get_rows(rows_pi).flex_ratios.push_back(0);
    }

    // Wrap in panel if border is set
    if (border_ != border_style::none) {
        detail::panel_data pd;
        pd.border = border_;
        pd.content = rows_id;
        auto panel_pi = s.add_panel(std::move(pd));
        auto panel_id = s.add_node(detail::widget_type::panel, panel_pi, detail::no_node, key_);
        if (z_order_ != 0) {
            s.set_z_order(panel_id, z_order_);
        }
        return panel_id;
    }

    if (z_order_ != 0) {
        s.set_z_order(rows_id, z_order_);
    }
    return rows_id;
}

} // namespace tapiru
