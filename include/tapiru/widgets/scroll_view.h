#pragma once

/**
 * @file scroll_view.h
 * @brief Fixed-size scrollable viewport widget builder.
 *
 * Renders child content into a clipped viewport with optional scrollbar.
 * Scroll offset is controlled externally via int pointers (like menu cursor).
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <memory>
#include <string_view>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    class TAPIRU_API scroll_view_builder {
      public:
        template <typename B>
        explicit scroll_view_builder(B &&content)
            : content_(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))) {}

        scroll_view_builder(scroll_view_builder &&) noexcept = default;
        scroll_view_builder &operator=(scroll_view_builder &&) noexcept = default;
        scroll_view_builder(const scroll_view_builder &) = delete;
        scroll_view_builder &operator=(const scroll_view_builder &) = delete;

        scroll_view_builder &viewport_width(uint32_t w) {
            view_w_ = w;
            return *this;
        }
        scroll_view_builder &viewport_height(uint32_t h) {
            view_h_ = h;
            return *this;
        }
        scroll_view_builder &scroll_x(const int *px) {
            scroll_x_ = px;
            return *this;
        }
        scroll_view_builder &scroll_y(const int *py) {
            scroll_y_ = py;
            return *this;
        }
        scroll_view_builder &show_scrollbar_v(bool v = true) {
            scrollbar_v_ = v;
            return *this;
        }
        scroll_view_builder &show_scrollbar_h(bool v = true) {
            scrollbar_h_ = v;
            return *this;
        }
        scroll_view_builder &border(border_style bs) {
            border_ = bs;
            return *this;
        }
        scroll_view_builder &border_style_override(const style &s) {
            border_sty_ = s;
            return *this;
        }
        scroll_view_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }
        scroll_view_builder &key(std::string_view k);

        node_id flatten_into(detail::scene &s) const;

      private:
        struct concept_ {
            virtual ~concept_() = default;
            virtual node_id flatten(detail::scene &s) const = 0;
        };
        template <typename B> struct model : concept_ {
            B builder;
            explicit model(B b) : builder(std::move(b)) {}
            node_id flatten(detail::scene &s) const override { return builder.flatten_into(s); }
        };
        std::unique_ptr<concept_> content_;
        uint32_t view_w_ = 0;
        uint32_t view_h_ = 0;
        const int *scroll_x_ = nullptr;
        const int *scroll_y_ = nullptr;
        bool scrollbar_v_ = true;
        bool scrollbar_h_ = false;
        border_style border_ = border_style::none;
        style border_sty_;
        int16_t z_order_ = 0;
        uint64_t key_ = 0;
    };

} // namespace tapiru
