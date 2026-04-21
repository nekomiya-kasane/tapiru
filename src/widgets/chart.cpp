/**
 * @file chart.cpp
 * @brief Braille chart widget implementations.
 */

#include "tapiru/widgets/chart.h"

#include "detail/scene.h"
#include "detail/widget_types.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tapiru {

// ── Braille dot layout ─────────────────────────────────────────────────
// Each braille character is a 2×4 dot grid. Dot positions map to bits:
//   (0,0)=0x01  (1,0)=0x08
//   (0,1)=0x02  (1,1)=0x10
//   (0,2)=0x04  (1,2)=0x20
//   (0,3)=0x40  (1,3)=0x80

static const uint8_t braille_left[4] = {0x01, 0x02, 0x04, 0x40};
static const uint8_t braille_right[4] = {0x08, 0x10, 0x20, 0x80};

static inline uint8_t braille_dot_bit(uint32_t dx, uint32_t dy) {
    return (dx == 0) ? braille_left[dy] : braille_right[dy];
}

// ── braille_grid ───────────────────────────────────────────────────────

braille_grid::braille_grid(uint32_t char_width, uint32_t char_height)
    : char_w_(char_width), char_h_(char_height), dots_(static_cast<size_t>(char_width) * char_height, 0) {}

void braille_grid::set(uint32_t dot_x, uint32_t dot_y) {
    if (dot_x >= dot_width() || dot_y >= dot_height()) {
        return;
    }
    uint32_t cx = dot_x / 2;
    uint32_t cy = dot_y / 4;
    uint32_t dx = dot_x % 2;
    uint32_t dy = dot_y % 4;
    dots_[cy * char_w_ + cx] |= braille_dot_bit(dx, dy);
}

void braille_grid::clear() {
    std::fill(dots_.begin(), dots_.end(), uint8_t(0));
}

std::string braille_grid::render() const {
    std::string result;
    for (uint32_t cy = 0; cy < char_h_; ++cy) {
        if (cy > 0) {
            result += '\n';
        }
        for (uint32_t cx = 0; cx < char_w_; ++cx) {
            // Braille base: U+2800
            char32_t cp = 0x2800 + dots_[cy * char_w_ + cx];
            // UTF-8 encode (U+2800..U+28FF are all 3-byte)
            result += static_cast<char>(0xE0 | (cp >> 12));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return result;
}

// ── line_chart_builder ─────────────────────────────────────────────────

node_id line_chart_builder::flatten_into(detail::scene &s) const {
    if (data_.empty() || width_ == 0 || height_ == 0) {
        return text_builder("").flatten_into(s);
    }

    float lo = has_min_ ? min_ : *std::min_element(data_.begin(), data_.end());
    float hi = has_max_ ? max_ : *std::max_element(data_.begin(), data_.end());
    if (hi <= lo) {
        hi = lo + 1.0f;
    }

    braille_grid grid(width_, height_);
    uint32_t dw = grid.dot_width();
    uint32_t dh = grid.dot_height();

    for (uint32_t dx = 0; dx < dw && dx < static_cast<uint32_t>(data_.size()); ++dx) {
        // Map data index
        size_t idx =
            static_cast<size_t>(static_cast<float>(dx) / static_cast<float>(dw) * static_cast<float>(data_.size()));
        if (idx >= data_.size()) {
            idx = data_.size() - 1;
        }

        float val = data_[idx];
        float norm = (val - lo) / (hi - lo);
        if (norm < 0.0f) {
            norm = 0.0f;
        }
        if (norm > 1.0f) {
            norm = 1.0f;
        }

        // Y is inverted: 0=top, dh-1=bottom
        uint32_t dy = static_cast<uint32_t>((1.0f - norm) * static_cast<float>(dh - 1) + 0.5f);
        grid.set(dx, dy);
    }

    auto tb = text_builder(grid.render());
    if (style_.fg.kind != tapiru::color_kind::default_color) {
        tb.style_override(style_);
    }
    return tb.flatten_into(s);
}

// ── bar_chart_builder ──────────────────────────────────────────────────

node_id bar_chart_builder::flatten_into(detail::scene &s) const {
    if (data_.empty()) {
        return text_builder("").flatten_into(s);
    }

    // Block characters: ▁▂▃▄▅▆▇█ (U+2581..U+2588)
    static const char *blocks[] = {
        " ",
        "\xe2\x96\x81",
        "\xe2\x96\x82",
        "\xe2\x96\x83",
        "\xe2\x96\x84",
        "\xe2\x96\x85",
        "\xe2\x96\x86",
        "\xe2\x96\x87",
        "\xe2\x96\x88",
    };

    float hi = *std::max_element(data_.begin(), data_.end());
    if (hi <= 0.0f) {
        hi = 1.0f;
    }

    // Determine column width: max of 1 (bar char) and longest label
    size_t col_w = 1;
    for (const auto &lbl : labels_) {
        if (lbl.size() > col_w) {
            col_w = lbl.size();
        }
    }

    // Build rows bottom-up
    std::string result;
    for (uint32_t row = max_height_; row > 0; --row) {
        if (row < max_height_) {
            result += '\n';
        }
        for (size_t i = 0; i < data_.size(); ++i) {
            float norm = data_[i] / hi;
            float bar_h = norm * static_cast<float>(max_height_);
            float level = bar_h - static_cast<float>(row - 1);
            int block_idx = 0;
            if (level >= 1.0f) {
                block_idx = 8; // full block
            } else if (level > 0.0f) {
                block_idx = static_cast<int>(level * 8.0f + 0.5f);
                if (block_idx < 1) {
                    block_idx = 1;
                }
                if (block_idx > 8) {
                    block_idx = 8;
                }
            }
            result += blocks[block_idx];
            // Pad bar character to column width
            for (size_t p = 1; p < col_w; ++p) {
                result += (block_idx > 0) ? blocks[block_idx] : " ";
            }
            if (i + 1 < data_.size()) {
                result += ' ';
            }
        }
    }

    // Add labels row if provided
    if (!labels_.empty()) {
        result += '\n';
        for (size_t i = 0; i < data_.size() && i < labels_.size(); ++i) {
            result += labels_[i];
            // Pad label to column width
            for (size_t p = labels_[i].size(); p < col_w; ++p) {
                result += ' ';
            }
            if (i + 1 < data_.size()) {
                result += ' ';
            }
        }
    }

    auto tb = text_builder(result);
    if (style_.fg.kind != tapiru::color_kind::default_color) {
        tb.style_override(style_);
    }
    return tb.flatten_into(s);
}

// ── scatter_builder ────────────────────────────────────────────────────

node_id scatter_builder::flatten_into(detail::scene &s) const {
    if (points_.empty() || width_ == 0 || height_ == 0) {
        return text_builder("").flatten_into(s);
    }

    // Find data bounds
    float x_lo = points_[0].x, x_hi = points_[0].x;
    float y_lo = points_[0].y, y_hi = points_[0].y;
    for (const auto &p : points_) {
        if (p.x < x_lo) {
            x_lo = p.x;
        }
        if (p.x > x_hi) {
            x_hi = p.x;
        }
        if (p.y < y_lo) {
            y_lo = p.y;
        }
        if (p.y > y_hi) {
            y_hi = p.y;
        }
    }
    if (x_hi <= x_lo) {
        x_hi = x_lo + 1.0f;
    }
    if (y_hi <= y_lo) {
        y_hi = y_lo + 1.0f;
    }

    braille_grid grid(width_, height_);
    uint32_t dw = grid.dot_width();
    uint32_t dh = grid.dot_height();

    for (const auto &p : points_) {
        float nx = (p.x - x_lo) / (x_hi - x_lo);
        float ny = (p.y - y_lo) / (y_hi - y_lo);
        uint32_t dx = static_cast<uint32_t>(nx * static_cast<float>(dw - 1) + 0.5f);
        uint32_t dy = static_cast<uint32_t>((1.0f - ny) * static_cast<float>(dh - 1) + 0.5f);
        grid.set(dx, dy);
    }

    auto tb = text_builder(grid.render());
    if (style_.fg.kind != tapiru::color_kind::default_color) {
        tb.style_override(style_);
    }
    return tb.flatten_into(s);
}

} // namespace tapiru
