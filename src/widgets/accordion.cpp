/**
 * @file accordion.cpp
 * @brief accordion_builder flatten_into implementation.
 */

#include "tapiru/widgets/accordion.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

accordion_builder &accordion_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id accordion_builder::flatten_into(detail::scene &s) const {
    // Compose as rows: for each section, add header + (optionally) content
    detail::rows_data rd;
    rd.gap = 0;

    auto pi = s.add_rows(std::move(rd));
    auto rows_id = s.add_node(detail::widget_type::rows, pi, detail::no_node, key_);
    if (z_order_ != 0) {
        s.set_z_order(rows_id, z_order_);
    }

    for (size_t i = 0; i < sections_.size(); ++i) {
        bool is_expanded = expanded_ && i < expanded_->size() && (*expanded_)[i];

        // Header: prefix with ▸ or ▾
        std::string prefix = is_expanded ? "\xe2\x96\xbe " : "\xe2\x96\xb8 ";
        std::string header_text = prefix + sections_[i].header;

        auto header_tb = text_builder(header_text);
        if (is_expanded) {
            header_tb.style_override(active_header_sty_);
        } else {
            header_tb.style_override(header_sty_);
        }

        if (border_ != border_style::none) {
            // Wrap header in a panel with border
            detail::panel_data pd;
            pd.border = border_;
            auto panel_pi = s.add_panel(std::move(pd));
            auto panel_id = s.add_node(detail::widget_type::panel, panel_pi, detail::no_node, 0);
            auto header_id = header_tb.flatten_into(s);
            s.get_panel(panel_pi).content = header_id;
            s.get_rows(pi).children.push_back(panel_id);
        } else {
            auto header_id = header_tb.flatten_into(s);
            s.get_rows(pi).children.push_back(header_id);
        }
        s.get_rows(pi).flex_ratios.push_back(0);

        // Content (only if expanded)
        if (is_expanded && sections_[i].content) {
            auto content_id = sections_[i].content->flatten(s);
            s.get_rows(pi).children.push_back(content_id);
            s.get_rows(pi).flex_ratios.push_back(0);
        }
    }

    return rows_id;
}

} // namespace tapiru
