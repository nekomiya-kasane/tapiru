/**
 * @file gauge.cpp
 * @brief Progress gauge element factory implementations.
 */

#include "tapiru/widgets/gauge.h"

#include "tapiru/widgets/builders.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tapiru {

    // Horizontal gauge block characters (eighths): ▏▎▍▌▋▊▉█
    static constexpr char32_t h_blocks[] = {U' ',      U'\x258F', U'\x258E', U'\x258D', U'\x258C',
                                            U'\x258B', U'\x258A', U'\x2589', U'\x2588'};

    // Vertical gauge block characters (eighths): ▁▂▃▄▅▆▇█
    static constexpr char32_t v_blocks[] = {U' ',      U'\x2581', U'\x2582', U'\x2583', U'\x2584',
                                            U'\x2585', U'\x2586', U'\x2587', U'\x2588'};

    element make_gauge(float progress) {
        return make_gauge(progress, style{}, style{color::default_color(), color::default_color(), attr::dim});
    }

    element make_gauge(float progress, const style &filled, const style &remaining) {
        float p = std::clamp(progress, 0.0f, 1.0f);

        // Build a text representation using block characters
        // We'll use a fixed internal width and let the layout system handle sizing
        constexpr uint32_t gauge_width = 20;
        float filled_cells = p * static_cast<float>(gauge_width);
        auto full_cells = static_cast<uint32_t>(filled_cells);
        float frac = filled_cells - static_cast<float>(full_cells);
        auto frac_idx = static_cast<uint32_t>(frac * 8.0f);
        if (frac_idx > 8) {
            frac_idx = 8;
        }

        std::string filled_str;
        for (uint32_t i = 0; i < full_cells && i < gauge_width; ++i) {
            // UTF-8 encode █ (U+2588)
            filled_str += "\xe2\x96\x88";
        }

        std::string partial_str;
        if (full_cells < gauge_width && frac_idx > 0) {
            // Encode the partial block character
            char32_t ch = h_blocks[frac_idx];
            if (ch < 0x80) {
                partial_str += static_cast<char>(ch);
            } else {
                partial_str += static_cast<char>(0xE0 | ((ch >> 12) & 0x0F));
                partial_str += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                partial_str += static_cast<char>(0x80 | (ch & 0x3F));
            }
        }

        uint32_t remaining_cells = gauge_width - full_cells - (frac_idx > 0 ? 1 : 0);
        std::string remaining_str;
        for (uint32_t i = 0; i < remaining_cells; ++i) {
            remaining_str += ' ';
        }

        // Build as a columns layout with styled text segments
        auto cb = columns_builder();
        if (!filled_str.empty()) {
            auto fb = text_builder(filled_str);
            fb.style_override(filled);
            cb.add(std::move(fb));
        }
        if (!partial_str.empty()) {
            auto pb = text_builder(partial_str);
            pb.style_override(filled);
            cb.add(std::move(pb));
        }
        if (!remaining_str.empty()) {
            auto rb = text_builder(remaining_str);
            rb.style_override(remaining);
            cb.add(std::move(rb));
        }
        cb.gap(0);
        return element(std::move(cb));
    }

    element make_gauge_direction(float progress, gauge_direction dir) {
        if (dir == gauge_direction::horizontal) {
            return make_gauge(progress);
        }

        // Vertical gauge
        float p = std::clamp(progress, 0.0f, 1.0f);
        constexpr uint32_t gauge_height = 10;
        float filled_cells = p * static_cast<float>(gauge_height);
        auto full_cells = static_cast<uint32_t>(filled_cells);
        float frac = filled_cells - static_cast<float>(full_cells);
        auto frac_idx = static_cast<uint32_t>(frac * 8.0f);
        if (frac_idx > 8) {
            frac_idx = 8;
        }

        auto rb = rows_builder();

        // Empty cells at top
        uint32_t empty_cells = gauge_height - full_cells - (frac_idx > 0 ? 1 : 0);
        for (uint32_t i = 0; i < empty_cells; ++i) {
            rb.add(text_builder(" "));
        }

        // Partial cell
        if (frac_idx > 0) {
            char32_t ch = v_blocks[frac_idx];
            std::string s;
            s += static_cast<char>(0xE0 | ((ch >> 12) & 0x0F));
            s += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
            s += static_cast<char>(0x80 | (ch & 0x3F));
            rb.add(text_builder(s));
        }

        // Full cells at bottom
        for (uint32_t i = 0; i < full_cells && i < gauge_height; ++i) {
            rb.add(text_builder("\xe2\x96\x88"));
        }

        return element(std::move(rb));
    }

} // namespace tapiru
