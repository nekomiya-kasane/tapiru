/**
 * @file canvas_widget.cpp
 * @brief Canvas widget builder implementation.
 */

#include "tapiru/widgets/canvas_widget.h"
#include "detail/scene.h"

#include <vector>

namespace tapiru {

struct canvas_widget_builder::impl {
    std::vector<detail::canvas_pixel>    points;
    std::vector<detail::canvas_line>     lines;
    std::vector<detail::canvas_circle>   circles;
    std::vector<detail::canvas_rect_cmd> rects;
    std::vector<detail::canvas_text_cmd> texts;
};

canvas_widget_builder::canvas_widget_builder(uint32_t pixel_w, uint32_t pixel_h)
    : impl_(std::make_shared<impl>()), pixel_w_(pixel_w), pixel_h_(pixel_h) {}

canvas_widget_builder& canvas_widget_builder::draw_point(int x, int y, const color& c) {
    impl_->points.push_back({x, y, c});
    return *this;
}

canvas_widget_builder& canvas_widget_builder::draw_line(int x1, int y1, int x2, int y2, const color& c) {
    impl_->lines.push_back({x1, y1, x2, y2, c, false});
    return *this;
}

canvas_widget_builder& canvas_widget_builder::draw_circle(int cx, int cy, int r, const color& c) {
    impl_->circles.push_back({cx, cy, r, c});
    return *this;
}

canvas_widget_builder& canvas_widget_builder::draw_rect(int x, int y, int w, int h, const color& c) {
    impl_->rects.push_back({x, y, w, h, c});
    return *this;
}

canvas_widget_builder& canvas_widget_builder::draw_text(int x, int y, std::string_view text, const style& s) {
    impl_->texts.push_back({x, y, std::string(text), s});
    return *this;
}

canvas_widget_builder& canvas_widget_builder::draw_block_line(int x1, int y1, int x2, int y2, const color& c) {
    impl_->lines.push_back({x1, y1, x2, y2, c, true});
    return *this;
}

node_id canvas_widget_builder::flatten_into(detail::scene& s) const {
    detail::canvas_widget_data data;
    data.pixel_w = pixel_w_;
    data.pixel_h = pixel_h_;
    if (impl_) {
        data.points  = impl_->points;
        data.lines   = impl_->lines;
        data.circles = impl_->circles;
        data.rects   = impl_->rects;
        data.texts   = impl_->texts;
    }
    auto pidx = s.add_canvas_widget(std::move(data));
    return s.add_node(detail::widget_type::canvas_widget, pidx);
}

element make_canvas(uint32_t w, uint32_t h, std::function<void(canvas_widget_builder&)> draw_fn) {
    canvas_widget_builder builder(w, h);
    if (draw_fn) draw_fn(builder);
    return element(std::move(builder));
}

}  // namespace tapiru
