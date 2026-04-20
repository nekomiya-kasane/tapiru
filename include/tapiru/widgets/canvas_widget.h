#pragma once

/**
 * @file canvas_widget.h
 * @brief Canvas drawing widget using braille/block characters.
 *
 * Provides a pixel-level drawing API that renders using Unicode braille
 * characters (2×4 pixels per cell) or half-block characters (1×2 pixels).
 *
 * Usage:
 *   auto cvs = canvas_widget_builder(80, 40);
 *   cvs.draw_point(10, 5, colors::red);
 *   cvs.draw_line(0, 0, 79, 39, colors::green);
 *   cvs.draw_circle(40, 20, 15, colors::blue);
 *   element e = std::move(cvs);
 *
 *   // Or via make_canvas:
 *   auto e = make_canvas(80, 40, [](canvas_widget_builder& c) {
 *       c.draw_line(0, 0, 79, 39);
 *   });
 */

#include <cstdint>
#include <functional>
#include <string_view>

#include "tapiru/core/element.h"
#include "tapiru/core/style.h"
#include "tapiru/exports.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

class TAPIRU_API canvas_widget_builder {
public:
    canvas_widget_builder(uint32_t pixel_w, uint32_t pixel_h);

    canvas_widget_builder& draw_point(int x, int y, const color& c = color::from_rgb(255, 255, 255));
    canvas_widget_builder& draw_line(int x1, int y1, int x2, int y2, const color& c = color::from_rgb(255, 255, 255));
    canvas_widget_builder& draw_circle(int cx, int cy, int r, const color& c = color::from_rgb(255, 255, 255));
    canvas_widget_builder& draw_rect(int x, int y, int w, int h, const color& c = color::from_rgb(255, 255, 255));
    canvas_widget_builder& draw_text(int x, int y, std::string_view text, const style& s = {});
    canvas_widget_builder& draw_block_line(int x1, int y1, int x2, int y2, const color& c = color::from_rgb(255, 255, 255));

    [[nodiscard]] uint32_t pixel_width() const noexcept { return pixel_w_; }
    [[nodiscard]] uint32_t pixel_height() const noexcept { return pixel_h_; }

    node_id flatten_into(detail::scene& s) const;

private:
    struct impl;
    std::shared_ptr<impl> impl_;
    uint32_t pixel_w_;
    uint32_t pixel_h_;
};

TAPIRU_API element make_canvas(uint32_t w, uint32_t h, std::function<void(canvas_widget_builder&)> draw_fn);

}  // namespace tapiru
