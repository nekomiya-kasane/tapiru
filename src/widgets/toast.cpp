/**
 * @file toast.cpp
 * @brief toast_builder flatten_into implementation.
 */

#include "tapiru/widgets/toast.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

toast_builder &toast_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id toast_builder::flatten_into(detail::scene &s) const {
    if (!state_ || !state_->visible()) {
        // Invisible toast — return an empty spacer
        return spacer_builder().flatten_into(s);
    }

    // Pick style based on level
    const style *sty = &info_sty_;
    std::string prefix;
    switch (state_->level()) {
    case toast_level::info:
        sty = &info_sty_;
        prefix = "\xe2\x84\xb9 ";
        break; // ℹ
    case toast_level::success:
        sty = &success_sty_;
        prefix = "\xe2\x9c\x93 ";
        break; // ✓
    case toast_level::warning:
        sty = &warning_sty_;
        prefix = "\xe2\x9a\xa0 ";
        break; // ⚠
    case toast_level::error:
        sty = &error_sty_;
        prefix = "\xe2\x9c\x97 ";
        break; // ✗
    }

    std::string msg = prefix + state_->message();
    auto tb = text_builder(msg);
    tb.style_override(*sty);

    if (border_ != border_style::none) {
        auto panel = panel_builder(std::move(tb));
        panel.border(border_);
        auto id = panel.flatten_into(s);
        if (z_order_ != 0) {
            s.set_z_order(id, z_order_);
        }
        return id;
    }

    auto id = tb.flatten_into(s);
    if (z_order_ != 0) {
        s.set_z_order(id, z_order_);
    }
    return id;
}

} // namespace tapiru
