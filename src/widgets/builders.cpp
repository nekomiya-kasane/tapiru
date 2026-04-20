/**
 * @file builders.cpp
 * @brief Builder flatten_into implementations.
 */

#include "tapiru/widgets/builders.h"
#include "detail/scene.h"

namespace tapiru {

// ── text_builder ────────────────────────────────────────────────────────

node_id text_builder::flatten_into(detail::scene& s) const {
    detail::text_data td;
    td.fragments = parse_markup(markup_);

    if (has_style_) {
        // Override all fragment styles
        for (auto& f : td.fragments) {
            f.sty = style_;
        }
    }

    td.align    = align_;
    td.overflow = overflow_;

    auto pi = s.add_text(std::move(td));
    auto id = s.add_node(detail::widget_type::text, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(id, z_order_);
    return id;
}

// ── rule_builder ────────────────────────────────────────────────────────

node_id rule_builder::flatten_into(detail::scene& s) const {
    detail::rule_data rd;
    rd.title    = title_;
    rd.rule_sty = style_;
    rd.align    = align_;
    rd.ch       = ch_;
    rd.gradient = gradient_;

    auto pi = s.add_rule(std::move(rd));
    auto id = s.add_node(detail::widget_type::rule, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(id, z_order_);
    return id;
}

// ── padding_builder ─────────────────────────────────────────────────────

node_id padding_builder::flatten_into(detail::scene& s) const {
    detail::padding_data pd;
    pd.top    = top_;
    pd.right  = right_;
    pd.bottom = bottom_;
    pd.left   = left_;

    auto pi = s.add_padding(std::move(pd));
    auto pad_id = s.add_node(detail::widget_type::padding, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(pad_id, z_order_);

    if (content_) {
        auto child_id = content_->flatten(s);
        s.get_padding(pi).content = child_id;
    }

    return pad_id;
}

// ── panel_builder ───────────────────────────────────────────────────────

node_id panel_builder::flatten_into(detail::scene& s) const {
    detail::panel_data pd;
    pd.border     = border_;
    pd.border_sty = border_sty_;
    pd.title      = title_;
    pd.title_sty  = title_sty_;
    pd.alpha      = alpha_;
    pd.bg_gradient = bg_gradient_;
    if (shadow_) {
        pd.shadow = detail::shadow_config{
            shadow_->offset_x, shadow_->offset_y, shadow_->blur,
            shadow_->shadow_color, shadow_->opacity
        };
    }
    if (shader_) pd.shader = shader_;

    auto pi = s.add_panel(std::move(pd));
    auto panel_id = s.add_node(detail::widget_type::panel, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(panel_id, z_order_);

    if (content_) {
        auto child_id = content_->flatten(s);
        s.get_panel(pi).content = child_id;
    }

    return panel_id;
}

// ── columns_builder ─────────────────────────────────────────────────────

node_id columns_builder::flatten_into(detail::scene& s) const {
    detail::columns_data cd;
    cd.gap   = gap_;
    cd.alpha = alpha_;

    auto pi = s.add_columns(std::move(cd));
    auto cols_id = s.add_node(detail::widget_type::columns, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(cols_id, z_order_);

    for (size_t i = 0; i < children_.size(); ++i) {
        auto child_id = children_[i]->flatten(s);
        s.get_columns(pi).children.push_back(child_id);
        s.get_columns(pi).flex_ratios.push_back(flex_ratios_[i]);
    }

    return cols_id;
}

// ── table_builder ───────────────────────────────────────────────────────

node_id table_builder::flatten_into(detail::scene& s) const {
    detail::table_data td;
    td.border      = border_;
    td.border_sty  = border_sty_;
    td.header_sty  = header_sty_;
    td.show_header = show_header_;
    if (shadow_) {
        td.shadow = detail::shadow_config{
            shadow_->offset_x, shadow_->offset_y, shadow_->blur,
            shadow_->shadow_color, shadow_->opacity
        };
    }
    td.border_gradient = border_gradient_;

    for (const auto& col : columns_) {
        td.column_defs.push_back({col.header, col.align, col.min_width, col.max_width});
    }

    size_t ncols = columns_.size();
    for (const auto& row : rows_) {
        for (size_t c = 0; c < ncols; ++c) {
            if (c < row.size()) {
                td.cells.push_back(parse_markup(row[c]));
            } else {
                td.cells.push_back({});
            }
        }
    }

    auto pi = s.add_table(std::move(td));
    auto id = s.add_node(detail::widget_type::table, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(id, z_order_);
    return id;
}

// ── overlay_builder ─────────────────────────────────────────────────────

node_id overlay_builder::flatten_into(detail::scene& s) const {
    detail::overlay_data od;
    od.alpha = alpha_;

    auto pi = s.add_overlay(std::move(od));
    auto ov_id = s.add_node(detail::widget_type::overlay, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(ov_id, z_order_);

    if (base_) {
        auto base_id = base_->flatten(s);
        s.get_overlay(pi).base = base_id;
    }
    if (over_) {
        auto over_id = over_->flatten(s);
        s.get_overlay(pi).overlay = over_id;
    }

    return ov_id;
}

// ── rows_builder ───────────────────────────────────────────────────────

node_id rows_builder::flatten_into(detail::scene& s) const {
    detail::rows_data rd;
    rd.gap   = gap_;
    rd.alpha = alpha_;

    auto pi = s.add_rows(std::move(rd));
    auto rows_id = s.add_node(detail::widget_type::rows, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(rows_id, z_order_);

    for (size_t i = 0; i < children_.size(); ++i) {
        auto child_id = children_[i]->flatten(s);
        s.get_rows(pi).children.push_back(child_id);
        s.get_rows(pi).flex_ratios.push_back(flex_ratios_[i]);
    }

    return rows_id;
}

// ── spacer_builder ─────────────────────────────────────────────────────

node_id spacer_builder::flatten_into(detail::scene& s) const {
    auto id = s.add_node(detail::widget_type::spacer, 0, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(id, z_order_);
    return id;
}

// ── sized_box_builder ──────────────────────────────────────────────────

node_id sized_box_builder::flatten_into(detail::scene& s) const {
    detail::sized_box_data sd;
    sd.fixed_w = fixed_w_;
    sd.fixed_h = fixed_h_;
    sd.min_w   = min_w_;
    sd.min_h   = min_h_;
    sd.max_w   = max_w_;
    sd.max_h   = max_h_;

    auto pi = s.add_sized_box(std::move(sd));
    auto id = s.add_node(detail::widget_type::sized_box, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(id, z_order_);

    if (content_) {
        auto child_id = content_->flatten(s);
        s.get_sized_box(pi).content = child_id;
    }

    return id;
}

// ── center_builder ────────────────────────────────────────────────────

node_id center_builder::flatten_into(detail::scene& s) const {
    detail::center_data cd;
    cd.horizontal = horizontal_;
    cd.vertical   = vertical_;

    auto pi = s.add_center(std::move(cd));
    auto id = s.add_node(detail::widget_type::center, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(id, z_order_);

    if (content_) {
        auto child_id = content_->flatten(s);
        s.get_center(pi).content = child_id;
    }

    return id;
}

}  // namespace tapiru
