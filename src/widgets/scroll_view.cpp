/**
 * @file scroll_view.cpp
 * @brief scroll_view_builder flatten_into implementation.
 */

#include "tapiru/widgets/scroll_view.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

scroll_view_builder &scroll_view_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id scroll_view_builder::flatten_into(detail::scene &s) const {
    detail::scroll_view_data sd;
    sd.scroll_x = scroll_x_;
    sd.scroll_y = scroll_y_;
    sd.view_w = view_w_;
    sd.view_h = view_h_;
    sd.show_scrollbar_v = scrollbar_v_;
    sd.show_scrollbar_h = scrollbar_h_;
    sd.border = border_;
    sd.border_sty = border_sty_;

    auto pi = s.add_scroll_view(std::move(sd));
    auto id = s.add_node(detail::widget_type::scroll_view, pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(id, z_order_);

    if (content_) {
        auto child_id = content_->flatten(s);
        s.get_scroll_view(pi).content = child_id;
    }

    return id;
}

} // namespace tapiru
