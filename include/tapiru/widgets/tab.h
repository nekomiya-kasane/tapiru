#pragma once

/**
 * @file tab.h
 * @brief Tab widget — a row of tab labels with switchable content panels.
 *
 * Composes columns_builder for the tab bar and rows_builder for layout.
 * Active tab index is controlled externally via a const int pointer.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    class TAPIRU_API tab_builder {
      public:
        tab_builder() = default;
        tab_builder(tab_builder &&) noexcept = default;
        tab_builder &operator=(tab_builder &&) noexcept = default;
        tab_builder(const tab_builder &) = delete;
        tab_builder &operator=(const tab_builder &) = delete;

        template <typename B> tab_builder &add_tab(std::string_view label, B &&content) {
            tabs_.push_back({std::string(label), std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))});
            return *this;
        }

        tab_builder &active(const int *idx) {
            active_ = idx;
            return *this;
        }
        tab_builder &tab_style(const style &s) {
            tab_sty_ = s;
            return *this;
        }
        tab_builder &active_tab_style(const style &s) {
            active_sty_ = s;
            return *this;
        }
        tab_builder &content_border(border_style bs) {
            content_border_ = bs;
            return *this;
        }
        tab_builder &content_border_style(const style &s) {
            content_border_sty_ = s;
            return *this;
        }
        tab_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }
        tab_builder &key(std::string_view k);

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

        struct tab_entry {
            std::string label;
            std::unique_ptr<concept_> content;
        };

        std::vector<tab_entry> tabs_;
        const int *active_ = nullptr;
        style tab_sty_;
        style active_sty_;
        border_style content_border_ = border_style::rounded;
        style content_border_sty_;
        int16_t z_order_ = 0;
        uint64_t key_ = 0;
    };

} // namespace tapiru
