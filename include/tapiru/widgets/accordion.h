#pragma once

/**
 * @file accordion.h
 * @brief Accordion widget — collapsible sections with headers.
 *
 * Each section has a header label and content. Only one (or multiple)
 * sections can be expanded at a time, controlled externally.
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

class TAPIRU_API accordion_builder {
  public:
    accordion_builder() = default;
    accordion_builder(accordion_builder &&) noexcept = default;
    accordion_builder &operator=(accordion_builder &&) noexcept = default;
    accordion_builder(const accordion_builder &) = delete;
    accordion_builder &operator=(const accordion_builder &) = delete;

    template <typename B> accordion_builder &add_section(std::string_view header, B &&content) {
        sections_.push_back({std::string(header), std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))});
        return *this;
    }

    accordion_builder &expanded(const std::vector<bool> *exp) {
        expanded_ = exp;
        return *this;
    }
    accordion_builder &header_style(const style &s) {
        header_sty_ = s;
        return *this;
    }
    accordion_builder &active_header_style(const style &s) {
        active_header_sty_ = s;
        return *this;
    }
    accordion_builder &border(border_style bs) {
        border_ = bs;
        return *this;
    }
    accordion_builder &z_order(int16_t z) {
        z_order_ = z;
        return *this;
    }
    accordion_builder &key(std::string_view k);

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

    struct section_entry {
        std::string header;
        std::unique_ptr<concept_> content;
    };

    std::vector<section_entry> sections_;
    const std::vector<bool> *expanded_ = nullptr;
    style header_sty_;
    style active_header_sty_;
    border_style border_ = border_style::none;
    int16_t z_order_ = 0;
    uint64_t key_ = 0;
};

} // namespace tapiru
