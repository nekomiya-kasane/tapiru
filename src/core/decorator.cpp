/**
 * @file decorator.cpp
 * @brief Predefined decorator factory implementations.
 */

#include "tapiru/core/decorator.h"

#include "tapiru/widgets/builders.h"

namespace tapiru {

    // ── Layout decorators ───────────────────────────────────────────────────

    decorator border(border_style bs) {
        return [bs](element inner) -> element {
            auto pb = panel_builder(std::move(inner));
            pb.border(bs);
            return element(std::move(pb));
        };
    }

    decorator padding(uint8_t all) {
        return [all](element inner) -> element {
            auto pb = padding_builder(std::move(inner));
            pb.pad(all);
            return element(std::move(pb));
        };
    }

    decorator padding(uint8_t v, uint8_t h) {
        return [v, h](element inner) -> element {
            auto pb = padding_builder(std::move(inner));
            pb.pad(v, h);
            return element(std::move(pb));
        };
    }

    decorator padding(uint8_t top, uint8_t right, uint8_t bottom, uint8_t left) {
        return [top, right, bottom, left](element inner) -> element {
            auto pb = padding_builder(std::move(inner));
            pb.pad(top, right, bottom, left);
            return element(std::move(pb));
        };
    }

    decorator size(uint32_t w, uint32_t h) {
        return [w, h](element inner) -> element {
            auto sb = sized_box_builder(std::move(inner));
            sb.width(w);
            sb.height(h);
            return element(std::move(sb));
        };
    }

    decorator min_size(uint32_t w, uint32_t h) {
        return [w, h](element inner) -> element {
            auto sb = sized_box_builder(std::move(inner));
            sb.min_width(w);
            sb.min_height(h);
            return element(std::move(sb));
        };
    }

    decorator max_size(uint32_t w, uint32_t h) {
        return [w, h](element inner) -> element {
            auto sb = sized_box_builder(std::move(inner));
            sb.max_width(w);
            sb.max_height(h);
            return element(std::move(sb));
        };
    }

    decorator flex(uint32_t ratio) {
        return [ratio](element inner) -> element {
            // flex wraps the element in a columns_builder with the given ratio.
            // When used inside a columns/rows container, the flex ratio is
            // picked up during layout. For standalone use, just pass through.
            (void)ratio; // TODO: wire flex ratio into scene data
            return inner;
        };
    }

    decorator center() {
        return [](element inner) -> element {
            auto cb = center_builder(std::move(inner));
            return element(std::move(cb));
        };
    }

    // ── Style decorators ────────────────────────────────────────────────────

    decorator fg_color(tapiru::color c) {
        return [c](element inner) -> element {
            style s;
            s.fg = c;
            return std::move(inner) | s;
        };
    }

    decorator bg_color(tapiru::color c) {
        return [c](element inner) -> element {
            style s;
            s.bg = c;
            return std::move(inner) | s;
        };
    }

    decorator bold() {
        return [](element inner) -> element {
            style s;
            s.attrs = attr::bold;
            return std::move(inner) | s;
        };
    }

    decorator dim() {
        return [](element inner) -> element {
            style s;
            s.attrs = attr::dim;
            return std::move(inner) | s;
        };
    }

    decorator italic() {
        return [](element inner) -> element {
            style s;
            s.attrs = attr::italic;
            return std::move(inner) | s;
        };
    }

    decorator underline() {
        return [](element inner) -> element {
            style s;
            s.attrs = attr::underline;
            return std::move(inner) | s;
        };
    }

    // ── Visual effect decorators ────────────────────────────────────────────

    decorator focus_indicator() {
        return [](element inner) -> element {
            // TODO: apply focused border highlight when component focus system is wired
            return inner;
        };
    }

    // ── Conditional decorators ──────────────────────────────────────────────

    decorator maybe(const bool *show) {
        return [show](element inner) -> element {
            if (show && !*show) {
                return element{};
            }
            return inner;
        };
    }

} // namespace tapiru
