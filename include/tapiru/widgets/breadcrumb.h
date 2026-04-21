#pragma once

/**
 * @file breadcrumb.h
 * @brief Breadcrumb navigation widget — a horizontal path display.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    class TAPIRU_API breadcrumb_builder {
      public:
        breadcrumb_builder() = default;

        breadcrumb_builder &add_item(std::string_view label) {
            items_.emplace_back(label);
            return *this;
        }

        breadcrumb_builder &separator(std::string_view sep) {
            separator_ = sep;
            return *this;
        }
        breadcrumb_builder &active(const int *idx) {
            active_ = idx;
            return *this;
        }
        breadcrumb_builder &item_style(const style &s) {
            item_sty_ = s;
            return *this;
        }
        breadcrumb_builder &active_style(const style &s) {
            active_sty_ = s;
            return *this;
        }
        breadcrumb_builder &separator_style(const style &s) {
            sep_sty_ = s;
            return *this;
        }
        breadcrumb_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }
        breadcrumb_builder &key(std::string_view k);

        node_id flatten_into(detail::scene &s) const;

      private:
        std::vector<std::string> items_;
        std::string separator_ = " \xe2\x80\xba "; // ›
        const int *active_ = nullptr;
        style item_sty_;
        style active_sty_;
        style sep_sty_;
        int16_t z_order_ = 0;
        uint64_t key_ = 0;
    };

} // namespace tapiru
