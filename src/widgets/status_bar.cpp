/**
 * @file status_bar.cpp
 * @brief status_bar_builder flatten_into implementation.
 */

#include "tapiru/widgets/status_bar.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

    status_bar_builder &status_bar_builder::key(std::string_view k) {
        key_ = detail::fnv1a_hash(k);
        return *this;
    }

    node_id status_bar_builder::flatten_into(detail::scene &s) const {
        // Build 3 text children: left, center, right
        auto left_text = text_builder(left_.empty() ? " " : left_).align(justify::left);
        auto center_text = text_builder(center_.empty() ? " " : center_).align(justify::center);
        auto right_text = text_builder(right_.empty() ? " " : right_).align(justify::right);

        if (has_style_) {
            left_text.style_override(bg_style_);
            center_text.style_override(bg_style_);
            right_text.style_override(bg_style_);
        }

        // Compose as columns with equal flex
        columns_builder cols;
        cols.add(std::move(left_text), 1).add(std::move(center_text), 1).add(std::move(right_text), 1).gap(0);

        // Wrap in a borderless panel for background fill
        if (has_style_) {
            panel_builder panel(std::move(cols));
            panel.border(border_style::none).border_style_override(bg_style_);
            // Constrain to exactly 1 row — status bar must never wrap
            sized_box_builder sb(std::move(panel));
            sb.height(1);
            if (key_ != 0) {
                sb.key(""); // key set below via scene
            }
            auto id = sb.flatten_into(s);
            if (key_ != 0) {
                s.set_key(id, key_);
            }
            if (z_order_ != 0) {
                s.set_z_order(id, z_order_);
            }
            return id;
        } else {
            sized_box_builder sb(std::move(cols));
            sb.height(1);
            auto id = sb.flatten_into(s);
            if (key_ != 0) {
                s.set_key(id, key_);
            }
            if (z_order_ != 0) {
                s.set_z_order(id, z_order_);
            }
            return id;
        }
    }

} // namespace tapiru
