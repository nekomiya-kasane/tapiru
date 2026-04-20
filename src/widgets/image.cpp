/**
 * @file image.cpp
 * @brief Image-to-character-art using half-block rendering.
 */

#include "tapiru/widgets/image.h"

#include "detail/scene.h"
#include "detail/widget_types.h"
#include "tapiru/widgets/builders.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace tapiru {

image_builder &image_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

// Nearest-neighbor sample from source image
static pixel_rgba sample(const std::vector<pixel_rgba> &px, uint32_t sw, uint32_t sh, uint32_t x, uint32_t y) {
    if (x >= sw || y >= sh) return {};
    return px[static_cast<size_t>(y) * sw + x];
}

node_id image_builder::flatten_into(detail::scene &s) const {
    if (pixels_.empty() || src_w_ == 0 || src_h_ == 0 || target_w_ == 0) {
        return text_builder("").flatten_into(s);
    }

    // Compute target dimensions preserving aspect ratio
    // Each terminal cell = 1 pixel wide, 2 pixels tall (half-block)
    uint32_t tw = target_w_;
    float aspect = static_cast<float>(src_h_) / static_cast<float>(src_w_);
    // Terminal chars are ~2:1 aspect, and we pack 2 rows per char
    uint32_t th_pixels = static_cast<uint32_t>(static_cast<float>(tw) * aspect + 0.5f);
    // Round up to even for half-block pairing
    if (th_pixels % 2 != 0) ++th_pixels;
    uint32_t th_chars = th_pixels / 2;
    if (th_chars == 0) th_chars = 1;

    // Build text_data directly with per-cell styled fragments.
    // Each cell is a ▀ (U+2580) with fg=top pixel, bg=bottom pixel.
    // This avoids the markup parser which would corrupt raw ANSI escapes.
    static const std::string half_block = "\xe2\x96\x80"; // ▀

    detail::text_data td;
    td.overflow = overflow_mode::truncate;

    for (uint32_t cy = 0; cy < th_chars; ++cy) {
        if (cy > 0) {
            td.fragments.push_back({"\n", style{}});
        }
        for (uint32_t cx = 0; cx < tw; ++cx) {
            // Map to source coordinates
            uint32_t sx =
                static_cast<uint32_t>(static_cast<float>(cx) / static_cast<float>(tw) * static_cast<float>(src_w_));
            uint32_t sy_top = static_cast<uint32_t>(static_cast<float>(cy * 2) / static_cast<float>(th_pixels) *
                                                    static_cast<float>(src_h_));
            uint32_t sy_bot = static_cast<uint32_t>(static_cast<float>(cy * 2 + 1) / static_cast<float>(th_pixels) *
                                                    static_cast<float>(src_h_));

            auto top = sample(pixels_, src_w_, src_h_, sx, sy_top);
            auto bot = sample(pixels_, src_w_, src_h_, sx, sy_bot);

            style cell_style;
            cell_style.fg = color::from_rgb(top.r, top.g, top.b);
            cell_style.bg = color::from_rgb(bot.r, bot.g, bot.b);

            td.fragments.push_back({half_block, cell_style});
        }
    }

    auto pi = s.add_text(std::move(td));
    return s.add_node(detail::widget_type::text, pi);
}

} // namespace tapiru
