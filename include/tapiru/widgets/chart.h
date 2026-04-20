#pragma once

/**
 * @file chart.h
 * @brief Braille-based terminal chart widgets.
 *
 * Uses Unicode Braille patterns U+2800–U+28FF for high-resolution plotting.
 * Each character cell encodes a 2×4 dot grid.
 *
 * Also provides bar_chart_builder using block characters ▁▂▃▄▅▆▇█.
 */

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "tapiru/exports.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

// ── Braille grid ───────────────────────────────────────────────────────

/**
 * @brief A 2D braille dot buffer.
 *
 * Each cell is 2 dots wide × 4 dots tall. The buffer stores dots
 * and converts to braille characters on demand.
 */
class TAPIRU_API braille_grid {
public:
    braille_grid(uint32_t char_width, uint32_t char_height);

    void set(uint32_t dot_x, uint32_t dot_y);
    void clear();

    [[nodiscard]] uint32_t dot_width() const noexcept { return char_w_ * 2; }
    [[nodiscard]] uint32_t dot_height() const noexcept { return char_h_ * 4; }
    [[nodiscard]] uint32_t char_width() const noexcept { return char_w_; }
    [[nodiscard]] uint32_t char_height() const noexcept { return char_h_; }

    [[nodiscard]] std::string render() const;

private:
    uint32_t char_w_;
    uint32_t char_h_;
    std::vector<uint8_t> dots_;  // char_w_ * char_h_ braille offsets
};

// ── Line chart ─────────────────────────────────────────────────────────

class TAPIRU_API line_chart_builder {
public:
    line_chart_builder(std::vector<float> data, uint32_t width, uint32_t height)
        : data_(std::move(data)), width_(width), height_(height) {}

    line_chart_builder& min_val(float v) { min_ = v; has_min_ = true; return *this; }
    line_chart_builder& max_val(float v) { max_ = v; has_max_ = true; return *this; }
    line_chart_builder& style_override(const style& s) { style_ = s; return *this; }
    line_chart_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    line_chart_builder& z_order(int16_t z) { z_order_ = z; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    std::vector<float> data_;
    uint32_t width_;
    uint32_t height_;
    float min_ = 0.0f;
    float max_ = 0.0f;
    bool has_min_ = false;
    bool has_max_ = false;
    style style_;
    uint64_t key_    = 0;
    int16_t  z_order_ = 0;
};

// ── Bar chart ──────────────────────────────────────────────────────────

class TAPIRU_API bar_chart_builder {
public:
    bar_chart_builder(std::vector<float> data, uint32_t max_height = 8)
        : data_(std::move(data)), max_height_(max_height) {}

    bar_chart_builder& labels(std::vector<std::string> lbl) { labels_ = std::move(lbl); return *this; }
    bar_chart_builder& style_override(const style& s) { style_ = s; return *this; }
    bar_chart_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    bar_chart_builder& z_order(int16_t z) { z_order_ = z; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    std::vector<float> data_;
    uint32_t max_height_;
    std::vector<std::string> labels_;
    style style_;
    uint64_t key_    = 0;
    int16_t  z_order_ = 0;
};

// ── Scatter plot ───────────────────────────────────────────────────────

struct scatter_point {
    float x;
    float y;
};

class TAPIRU_API scatter_builder {
public:
    scatter_builder(std::vector<scatter_point> points, uint32_t width, uint32_t height)
        : points_(std::move(points)), width_(width), height_(height) {}

    scatter_builder& style_override(const style& s) { style_ = s; return *this; }
    scatter_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    scatter_builder& z_order(int16_t z) { z_order_ = z; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    std::vector<scatter_point> points_;
    uint32_t width_;
    uint32_t height_;
    style style_;
    uint64_t key_    = 0;
    int16_t  z_order_ = 0;
};

}  // namespace tapiru
