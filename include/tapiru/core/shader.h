#pragma once

/**
 * @file shader.h
 * @brief Terminal shader system: per-region post-processing callbacks.
 *
 * Shaders run after widget rendering but before border_join,
 * allowing dynamic visual effects on terminal cell grids.
 */

#include "tapiru/core/canvas.h"
#include "tapiru/core/cell.h"
#include "tapiru/core/style.h"
#include "tapiru/core/style_table.h"
#include "tapiru/layout/types.h"

#include <cmath>
#include <cstdint>
#include <functional>

namespace tapiru {

/**
 * @brief Shader function signature.
 * @param cv         canvas to read/write cells
 * @param styles     style table for interning new styles
 * @param region     the rect this shader operates on
 * @param frame_time elapsed time in seconds (for animation)
 */
using shader_fn = std::function<void(canvas &cv, style_table &styles, rect region, double frame_time)>;

namespace shaders {

/**
 * @brief CRT scanline effect: dims every other row.
 * @param intensity  0.0 = no effect, 1.0 = fully black alternate rows
 */
inline shader_fn scanline(float intensity = 0.3f) {
    return [intensity](canvas &cv, style_table &styles, rect region, double /*frame_time*/) {
        style dark_sty;
        dark_sty.bg = color::from_rgb(0, 0, 0);
        auto d_sid = styles.intern(dark_sty);
        uint8_t alpha = static_cast<uint8_t>(intensity * 128.0f);
        if (alpha == 0) return;
        for (uint32_t y = region.y; y < region.y + region.h; ++y) {
            if ((y - region.y) % 2 == 0) continue; // dim odd rows
            for (uint32_t x = region.x; x < region.x + region.w; ++x) {
                if (x >= cv.width() || y >= cv.height()) continue;
                cv.set_blended(x, y, cell{U' ', d_sid, 1, alpha}, styles);
            }
        }
    };
}

/**
 * @brief Animated diagonal shimmer highlight.
 * @param c     highlight color
 * @param speed sweep speed (cycles per second)
 */
inline shader_fn shimmer(color c, float speed = 2.0f) {
    return [c, speed](canvas &cv, style_table &styles, rect region, double frame_time) {
        float phase = static_cast<float>(std::fmod(frame_time * speed, 1.0));
        float band_pos = phase * static_cast<float>(region.w + region.h);

        style highlight_sty;
        highlight_sty.fg = c;
        auto h_sid = styles.intern(highlight_sty);

        for (uint32_t y = region.y; y < region.y + region.h; ++y) {
            for (uint32_t x = region.x; x < region.x + region.w; ++x) {
                if (x >= cv.width() || y >= cv.height()) continue;
                float diag = static_cast<float>((x - region.x) + (y - region.y));
                float dist = std::abs(diag - band_pos);
                if (dist < 3.0f) {
                    uint8_t alpha = static_cast<uint8_t>(80.0f * (1.0f - dist / 3.0f));
                    cv.set_blended(x, y, cell{U' ', h_sid, 1, alpha}, styles);
                }
            }
        }
    };
}

/**
 * @brief Vignette: darken cells near the edges of the region.
 * @param strength  0.0 = no effect, 1.0 = edges fully black
 */
inline shader_fn vignette(float strength = 0.5f) {
    return [strength](canvas &cv, style_table &styles, rect region, double /*frame_time*/) {
        style dark_sty;
        dark_sty.bg = color::from_rgb(0, 0, 0);
        auto d_sid = styles.intern(dark_sty);

        float hw = static_cast<float>(region.w) * 0.5f;
        float hh = static_cast<float>(region.h) * 0.5f;

        for (uint32_t y = region.y; y < region.y + region.h; ++y) {
            for (uint32_t x = region.x; x < region.x + region.w; ++x) {
                if (x >= cv.width() || y >= cv.height()) continue;
                float dx = (static_cast<float>(x - region.x) - hw) / hw;
                float dy = (static_cast<float>(y - region.y) - hh) / hh;
                // Aspect ratio correction: terminal cells are ~2:1
                dy *= 2.0f;
                float dist = dx * dx + dy * dy;
                if (dist > 1.0f) dist = 1.0f;
                uint8_t alpha = static_cast<uint8_t>(strength * dist * 128.0f);
                if (alpha > 0) {
                    cv.set_blended(x, y, cell{U' ', d_sid, 1, alpha}, styles);
                }
            }
        }
    };
}

/**
 * @brief Pulsing glow effect on the border region.
 * @param c    glow color
 * @param bpm  pulses per minute
 */
inline shader_fn glow_pulse(color c, float bpm = 60.0f) {
    return [c, bpm](canvas &cv, style_table &styles, rect region, double frame_time) {
        float phase = static_cast<float>(std::fmod(frame_time * bpm / 60.0, 1.0));
        float pulse = 0.5f + 0.5f * std::sin(phase * 2.0f * 3.14159265f);

        style glow_sty;
        glow_sty.bg = c;
        auto g_sid = styles.intern(glow_sty);

        uint8_t alpha = static_cast<uint8_t>(pulse * 80.0f);
        if (alpha == 0) return;

        // Glow on border cells only (1-cell border)
        for (uint32_t x = region.x; x < region.x + region.w; ++x) {
            if (x < cv.width()) {
                if (region.y < cv.height()) cv.set_blended(x, region.y, cell{U' ', g_sid, 1, alpha}, styles);
                if (region.y + region.h > 0 && region.y + region.h - 1 < cv.height())
                    cv.set_blended(x, region.y + region.h - 1, cell{U' ', g_sid, 1, alpha}, styles);
            }
        }
        for (uint32_t y = region.y + 1; y + 1 < region.y + region.h; ++y) {
            if (y < cv.height()) {
                if (region.x < cv.width()) cv.set_blended(region.x, y, cell{U' ', g_sid, 1, alpha}, styles);
                if (region.x + region.w > 0 && region.x + region.w - 1 < cv.width())
                    cv.set_blended(region.x + region.w - 1, y, cell{U' ', g_sid, 1, alpha}, styles);
            }
        }
    };
}

} // namespace shaders
} // namespace tapiru
