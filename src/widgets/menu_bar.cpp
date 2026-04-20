/**
 * @file menu_bar.cpp
 * @brief menu_bar_builder flatten_into implementation.
 */

#include "tapiru/widgets/menu_bar.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

menu_bar_builder &menu_bar_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id menu_bar_builder::flatten_into(detail::scene &s) const {
    // The menu bar is a composite: an overlay with the bar as base
    // and the active dropdown as the overlay layer.

    // Build the horizontal bar as a columns_builder with text items
    auto bar = columns_builder();
    int active = state_ ? state_->active_index() : -1;
    bool is_open = state_ ? state_->is_open() : false;

    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        std::string label = " " + entries_[static_cast<size_t>(i)].label + " ";
        bool is_active = (i == active && is_open);
        auto item = text_builder(label);
        if (is_active) {
            item.style_override(active_sty_);
        } else {
            item.style_override(bar_sty_);
        }
        bar.add(std::move(item));
    }

    // If no dropdown is open, just return the bar
    if (!is_open || active < 0 || active >= static_cast<int>(entries_.size())) {
        auto bar_id = bar.flatten_into(s);
        if (z_order_ != 0) s.set_z_order(bar_id, z_order_);
        return bar_id;
    }

    // Build the dropdown menu for the active entry
    const auto &entry = entries_[static_cast<size_t>(active)];
    auto dropdown = menu_builder();
    for (const auto &item : entry.items) {
        dropdown.add(item);
    }
    dropdown.border(dropdown_border_);
    dropdown.highlight_style(highlight_sty_);
    dropdown.shortcut_style(shortcut_sty_);
    dropdown.item_style(item_sty_);
    if (state_) {
        dropdown.state(&state_->dropdown_state());
    }
    dropdown.z_order(z_order_ + 1);

    // Compose bar + dropdown using overlay
    auto ov = overlay_builder(std::move(bar), std::move(dropdown));
    if (z_order_ != 0) ov.z_order(z_order_);

    return ov.flatten_into(s);
}

} // namespace tapiru
