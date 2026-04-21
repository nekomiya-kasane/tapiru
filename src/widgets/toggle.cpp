/**
 * @file toggle.cpp
 * @brief toggle_builder flatten_into implementation.
 */

#include "tapiru/widgets/toggle.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

toggle_builder &toggle_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id toggle_builder::flatten_into(detail::scene &s) const {
    bool is_on = value_ ? *value_ : false;

    std::string switch_text = is_on ? on_text_ : off_text_;
    std::string full_text = switch_text;
    if (!label_.empty()) {
        full_text += " " + label_;
    }

    // Build as columns: [switch] [label]
    auto cols = columns_builder();

    auto switch_tb = text_builder(switch_text);
    switch_tb.style_override(is_on ? on_sty_ : off_sty_);
    cols.add(std::move(switch_tb));

    if (!label_.empty()) {
        auto label_tb = text_builder(" " + label_);
        label_tb.style_override(label_sty_);
        cols.add(std::move(label_tb));
    }

    auto id = cols.flatten_into(s);
    if (z_order_ != 0) {
        s.set_z_order(id, z_order_);
    }
    return id;
}

} // namespace tapiru
