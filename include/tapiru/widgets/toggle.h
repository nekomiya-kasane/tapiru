#pragma once

/**
 * @file toggle.h
 * @brief Toggle switch widget — an on/off switch with label.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <string>
#include <string_view>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    class TAPIRU_API toggle_builder {
      public:
        toggle_builder() = default;

        toggle_builder &value(const bool *v) {
            value_ = v;
            return *this;
        }
        toggle_builder &label(std::string_view l) {
            label_ = l;
            return *this;
        }
        toggle_builder &on_style(const style &s) {
            on_sty_ = s;
            return *this;
        }
        toggle_builder &off_style(const style &s) {
            off_sty_ = s;
            return *this;
        }
        toggle_builder &label_style(const style &s) {
            label_sty_ = s;
            return *this;
        }
        toggle_builder &on_text(std::string_view t) {
            on_text_ = t;
            return *this;
        }
        toggle_builder &off_text(std::string_view t) {
            off_text_ = t;
            return *this;
        }
        toggle_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }
        toggle_builder &key(std::string_view k);

        node_id flatten_into(detail::scene &s) const;

      private:
        const bool *value_ = nullptr;
        std::string label_;
        style on_sty_;
        style off_sty_;
        style label_sty_;
        std::string on_text_ = "\xe2\x97\x89";  // ◉
        std::string off_text_ = "\xe2\x97\x8b"; // ○
        int16_t z_order_ = 0;
        uint64_t key_ = 0;
    };

} // namespace tapiru
