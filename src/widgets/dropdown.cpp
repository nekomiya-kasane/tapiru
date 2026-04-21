/**
 * @file dropdown.cpp
 * @brief dropdown_builder flatten_into implementation.
 */

#include "tapiru/widgets/dropdown.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/menu.h"

namespace tapiru {

dropdown_builder &dropdown_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id dropdown_builder::flatten_into(detail::scene &s) const {
    int sel = selected_ ? *selected_ : -1;
    bool is_open = open_ ? *open_ : false;

    // Button label: selected option or placeholder
    std::string label;
    if (sel >= 0 && sel < static_cast<int>(options_.size())) {
        label = options_[static_cast<size_t>(sel)];
    } else {
        label = placeholder_;
    }

    // Add dropdown arrow
    std::string button_text = label + (is_open ? " \xe2\x96\xb4" : " \xe2\x96\xbe"); // ▴ or ▾

    if (!is_open) {
        // Just show the button (optionally in a panel)
        if (border_ != border_style::none) {
            auto tb = text_builder(button_text);
            tb.style_override(button_sty_);
            auto panel = panel_builder(std::move(tb));
            panel.border(border_);
            auto id = panel.flatten_into(s);
            if (z_order_ != 0) {
                s.set_z_order(id, z_order_);
            }
            return id;
        }
        auto tb = text_builder(button_text);
        tb.style_override(button_sty_);
        auto id = tb.flatten_into(s);
        if (z_order_ != 0) {
            s.set_z_order(id, z_order_);
        }
        return id;
    }

    // Open: compose button + dropdown list using rows (button on top, menu below)
    auto menu = menu_builder();
    for (const auto &opt : options_) {
        menu.add_item(opt);
    }
    menu.border(border_);
    menu.highlight_style(highlight_sty_);
    menu.item_style(item_sty_);
    if (selected_) {
        menu.cursor(selected_);
    }
    menu.z_order(z_order_ + 1);

    // Build as rows: [button] [menu]
    auto tb = text_builder(button_text);
    tb.style_override(button_sty_);

    auto rows = rows_builder();
    if (border_ != border_style::none) {
        auto bp = panel_builder(std::move(tb));
        bp.border(border_);
        rows.add(std::move(bp));
    } else {
        rows.add(std::move(tb));
    }
    rows.add(std::move(menu));

    auto id = rows.flatten_into(s);
    if (z_order_ != 0) {
        s.set_z_order(id, z_order_);
    }
    return id;
}

} // namespace tapiru
