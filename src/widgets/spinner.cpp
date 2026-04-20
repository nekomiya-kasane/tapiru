/**
 * @file spinner.cpp
 * @brief Spinner builder flatten_into implementation.
 */

#include "tapiru/widgets/spinner.h"
#include "tapiru/widgets/builders.h"
#include "detail/scene.h"

namespace tapiru {

spinner_builder& spinner_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id spinner_builder::flatten_into(detail::scene& s) const {
    detail::text_data td;

    if (state_->done()) {
        // Show done text + message (parse markup in done_text_)
        auto done_frags = parse_markup(done_text_);
        for (auto& f : done_frags) {
            td.fragments.push_back(std::move(f));
        }
        if (!message_.empty()) {
            td.fragments.push_back({" ", style{}});
            td.fragments.push_back({message_, style{}});
        }
    } else {
        // Show current spinner frame + message
        state_->tick();
        uint32_t idx = state_->frame();
        if (!frames_.empty()) {
            idx = idx % static_cast<uint32_t>(frames_.size());
            td.fragments.push_back({frames_[idx], spinner_sty_});
        }
        if (!message_.empty()) {
            td.fragments.push_back({" ", style{}});
            td.fragments.push_back({message_, style{}});
        }
    }

    td.overflow = overflow_mode::truncate;
    auto pi = s.add_text(std::move(td));
    return s.add_node(detail::widget_type::text, pi);
}

}  // namespace tapiru
