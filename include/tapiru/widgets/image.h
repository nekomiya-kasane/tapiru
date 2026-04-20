#pragma once

/**
 * @file image.h
 * @brief Image-to-character-art widget using half-block rendering.
 *
 * Uses ▀ (U+2580) with fg=top pixel, bg=bottom pixel to achieve
 * 2× vertical resolution. Each terminal cell represents 2 vertical pixels.
 *
 * Usage:
 *   std::vector<uint32_t> pixels = ...;  // RGBA packed
 *   con.print_widget(image_builder(pixels, img_w, img_h).target_width(40), 80);
 */

#include <cstdint>
#include <vector>

#include "tapiru/exports.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

/**
 * @brief Pixel data: packed RGBA (R in low byte).
 */
struct TAPIRU_API pixel_rgba {
    uint8_t r = 0, g = 0, b = 0, a = 255;
};

/**
 * @brief Builder that converts pixel data to half-block character art.
 */
class TAPIRU_API image_builder {
public:
    image_builder(const std::vector<pixel_rgba>& pixels, uint32_t width, uint32_t height)
        : pixels_(pixels), src_w_(width), src_h_(height) {}

    image_builder& target_width(uint32_t w) { target_w_ = w; return *this; }
    image_builder& key(std::string_view k);
    image_builder& z_order(int16_t z) { z_order_ = z; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    std::vector<pixel_rgba> pixels_;
    uint32_t src_w_ = 0;
    uint32_t src_h_ = 0;
    uint32_t target_w_ = 40;
    uint64_t key_    = 0;
    int16_t  z_order_ = 0;
};

}  // namespace tapiru
