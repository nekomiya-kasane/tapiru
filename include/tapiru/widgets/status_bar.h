#pragma once

/**
 * @file status_bar.h
 * @brief Status bar widget — a single-row bar with left/center/right sections.
 *
 * Composite builder: internally flattens to panel + columns + text.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"

#include <string>
#include <string_view>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    class TAPIRU_API status_bar_builder {
      public:
        status_bar_builder() = default;

        status_bar_builder &left(std::string_view markup) {
            left_ = markup;
            return *this;
        }
        status_bar_builder &center(std::string_view markup) {
            center_ = markup;
            return *this;
        }
        status_bar_builder &right(std::string_view markup) {
            right_ = markup;
            return *this;
        }
        status_bar_builder &style_override(const style &s) {
            bg_style_ = s;
            has_style_ = true;
            return *this;
        }
        status_bar_builder &key(std::string_view k);
        status_bar_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }

        node_id flatten_into(detail::scene &s) const;

      private:
        std::string left_, center_, right_;
        style bg_style_;
        bool has_style_ = false;
        int16_t z_order_ = 0;
        uint64_t key_ = 0;
    };

} // namespace tapiru
