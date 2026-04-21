/**
 * @file popup.cpp
 * @brief popup_builder flatten_into implementation.
 *
 * Composes overlay_builder with a dim shader layer and centered content
 * using rows_builder + columns_builder + spacer_builder.
 */

#include "tapiru/widgets/popup.h"

#include "detail/scene.h"
#include "tapiru/core/shader.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

    popup_builder &popup_builder::key(std::string_view k) {
        key_ = detail::fnv1a_hash(k);
        return *this;
    }

    node_id popup_builder::flatten_into(detail::scene &s) const {
        // 1. Flatten content directly, then wrap in a panel node manually
        auto content_id = content_->flatten(s);

        detail::panel_data pd;
        pd.border = border_;
        pd.border_sty = border_sty_;
        pd.title = title_;
        pd.content = content_id;
        if (shadow_) {
            pd.shadow = detail::shadow_config{shadow_->offset_x, shadow_->offset_y, shadow_->blur,
                                              shadow_->shadow_color, shadow_->opacity};
        }

        auto panel_pi = s.add_panel(std::move(pd));
        auto panel_id = s.add_node(detail::widget_type::panel, panel_pi);

        // 2. Center the content panel using spacer-based flex layout
        //    rows { top_spacer, columns { left_spacer, panel, right_spacer }, bottom_spacer }

        // Build inner columns: spacer | panel | spacer
        detail::columns_data cd;
        cd.gap = 0;
        auto spacer1 = s.add_node(detail::widget_type::spacer, 0);
        auto spacer2 = s.add_node(detail::widget_type::spacer, 0);
        cd.children = {spacer1, panel_id, spacer2};
        cd.flex_ratios = {1, 0, 1};
        auto cols_pi = s.add_columns(std::move(cd));
        auto cols_id = s.add_node(detail::widget_type::columns, cols_pi);

        // Build outer rows based on anchor
        detail::rows_data rd;
        rd.gap = 0;
        switch (anchor_) {
        case popup_anchor::top: {
            auto sp_bot = s.add_node(detail::widget_type::spacer, 0);
            rd.children = {cols_id, sp_bot};
            rd.flex_ratios = {0, 1};
            break;
        }
        case popup_anchor::bottom: {
            auto sp_top = s.add_node(detail::widget_type::spacer, 0);
            rd.children = {sp_top, cols_id};
            rd.flex_ratios = {1, 0};
            break;
        }
        case popup_anchor::center:
        default: {
            auto sp_top = s.add_node(detail::widget_type::spacer, 0);
            auto sp_bot = s.add_node(detail::widget_type::spacer, 0);
            rd.children = {sp_top, cols_id, sp_bot};
            rd.flex_ratios = {1, 0, 1};
            break;
        }
        }
        auto rows_pi = s.add_rows(std::move(rd));
        auto rows_id = s.add_node(detail::widget_type::rows, rows_pi);

        // 3. Build the dim background layer
        detail::panel_data dim_pd;
        dim_pd.border = border_style::none;
        dim_pd.alpha = static_cast<uint8_t>(dim_ < 0.f ? 0 : dim_ > 1.f ? 255 : dim_ * 255.f);
        dim_pd.border_sty.bg = color::from_rgb(0, 0, 0);
        // Dim panel has no content — just a dark background fill
        dim_pd.bg_gradient = std::nullopt;
        auto dim_pi = s.add_panel(std::move(dim_pd));
        auto dim_id = s.add_node(detail::widget_type::panel, dim_pi);

        // 4. Compose: overlay(dim_background, centered_content)
        detail::overlay_data od;
        od.base = dim_id;
        od.overlay = rows_id;
        auto ov_pi = s.add_overlay(std::move(od));
        auto ov_id = s.add_node(detail::widget_type::overlay, ov_pi, detail::no_node, key_);
        if (z_order_ != 0) {
            s.set_z_order(ov_id, z_order_);
        }
        return ov_id;
    }

} // namespace tapiru
