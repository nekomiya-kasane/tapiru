/**
 * @file tab.cpp
 * @brief tab_builder flatten_into implementation.
 */

#include "tapiru/widgets/tab.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

    tab_builder &tab_builder::key(std::string_view k) {
        key_ = detail::fnv1a_hash(k);
        return *this;
    }

    node_id tab_builder::flatten_into(detail::scene &s) const {
        if (tabs_.empty()) {
            return text_builder("").flatten_into(s);
        }

        int active_idx = active_ ? *active_ : 0;
        if (active_idx < 0 || active_idx >= static_cast<int>(tabs_.size())) {
            active_idx = 0;
        }

        // Build tab bar as a columns_builder
        auto tab_bar = columns_builder();
        for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
            std::string label = " " + tabs_[static_cast<size_t>(i)].label + " ";
            bool is_active = (i == active_idx);
            auto item = text_builder(label);
            if (is_active) {
                item.style_override(active_sty_);
            } else {
                item.style_override(tab_sty_);
            }
            tab_bar.add(std::move(item));
        }

        // Build the content panel for the active tab
        const auto &active_tab = tabs_[static_cast<size_t>(active_idx)];
        if (!active_tab.content) {
            auto bar_id = tab_bar.flatten_into(s);
            if (z_order_ != 0) {
                s.set_z_order(bar_id, z_order_);
            }
            return bar_id;
        }

        if (content_border_ != border_style::none) {
            detail::rows_data rd;
            rd.gap = 0;

            auto pi = s.add_rows(std::move(rd));
            auto rows_id = s.add_node(detail::widget_type::rows, pi, detail::no_node, key_);
            if (z_order_ != 0) {
                s.set_z_order(rows_id, z_order_);
            }

            // Flatten tab bar as first child
            auto bar_id = tab_bar.flatten_into(s);
            s.get_rows(pi).children.push_back(bar_id);
            s.get_rows(pi).flex_ratios.push_back(0);

            // Flatten content inside a panel as second child
            detail::panel_data pd;
            pd.border = content_border_;
            pd.border_sty = content_border_sty_;
            auto panel_pi = s.add_panel(std::move(pd));
            auto panel_id = s.add_node(detail::widget_type::panel, panel_pi, detail::no_node, 0);

            auto content_id = active_tab.content->flatten(s);
            s.get_panel(panel_pi).content = content_id;

            s.get_rows(pi).children.push_back(panel_id);
            s.get_rows(pi).flex_ratios.push_back(1);

            return rows_id;
        } else {
            // No border — just rows(tab_bar, content)
            detail::rows_data rd;
            rd.gap = 0;

            auto pi = s.add_rows(std::move(rd));
            auto rows_id = s.add_node(detail::widget_type::rows, pi, detail::no_node, key_);
            if (z_order_ != 0) {
                s.set_z_order(rows_id, z_order_);
            }

            auto bar_id = tab_bar.flatten_into(s);
            s.get_rows(pi).children.push_back(bar_id);
            s.get_rows(pi).flex_ratios.push_back(0);

            auto content_id = active_tab.content->flatten(s);
            s.get_rows(pi).children.push_back(content_id);
            s.get_rows(pi).flex_ratios.push_back(1);

            return rows_id;
        }
    }

} // namespace tapiru
