#pragma once

/**
 * @file popup.h
 * @brief Popup widget — a modal overlay with dimmed background and centered content.
 *
 * Composite builder: internally flattens to overlay + rows/columns centering.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace tapiru {

namespace detail {
class scene;
}
using node_id = uint32_t;

enum class popup_anchor : uint8_t { center, top, bottom };

class TAPIRU_API popup_builder {
  public:
    template <typename B>
    explicit popup_builder(B &&content)
        : content_(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(content))) {}

    popup_builder(popup_builder &&) noexcept = default;
    popup_builder &operator=(popup_builder &&) noexcept = default;
    popup_builder(const popup_builder &) = delete;
    popup_builder &operator=(const popup_builder &) = delete;

    popup_builder &anchor(popup_anchor a) {
        anchor_ = a;
        return *this;
    }
    popup_builder &dim_background(float opacity) {
        dim_ = opacity;
        return *this;
    }
    popup_builder &border(border_style bs) {
        border_ = bs;
        return *this;
    }
    popup_builder &border_style_override(const style &s) {
        border_sty_ = s;
        return *this;
    }
    popup_builder &title(std::string_view t) {
        title_ = t;
        return *this;
    }
    popup_builder &z_order(int16_t z) {
        z_order_ = z;
        return *this;
    }
    popup_builder &key(std::string_view k);

    popup_builder &shadow(int8_t offset_x = 2, int8_t offset_y = 1, uint8_t blur = 1,
                          color shadow_color = color::from_rgb(0, 0, 0), uint8_t opacity = 128) {
        shadow_ = {offset_x, offset_y, blur, shadow_color, opacity};
        return *this;
    }
    popup_builder &glow(color glow_color, uint8_t blur = 1, uint8_t opacity = 100) {
        shadow_ = {0, 0, blur, glow_color, opacity};
        return *this;
    }

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

    struct shadow_cfg {
        int8_t offset_x = 2;
        int8_t offset_y = 1;
        uint8_t blur = 1;
        color shadow_color = color::from_rgb(0, 0, 0);
        uint8_t opacity = 128;
    };

    std::unique_ptr<concept_> content_;
    popup_anchor anchor_ = popup_anchor::center;
    float dim_ = 0.5f;
    border_style border_ = border_style::rounded;
    style border_sty_;
    std::string title_;
    int16_t z_order_ = 0;
    uint64_t key_ = 0;
    std::optional<shadow_cfg> shadow_;
};

} // namespace tapiru
