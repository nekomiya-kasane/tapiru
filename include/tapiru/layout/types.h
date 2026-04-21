#pragma once

/**
 * @file types.h
 * @brief Layout types -- geometry and borders forwarded from tapioca,
 *        layout-specific enums defined here.
 */

#include "tapioca/layout_types.h"
#include "tapiru/exports.h"

#include <cstdint>

namespace tapiru {

    // ── Re-export geometry / border types from tapioca ──────────────────────

    using tapioca::border_chars;
    using tapioca::border_style;
    using tapioca::box_constraints;
    using tapioca::get_border_chars;
    using tapioca::measurement;
    using tapioca::rect;
    using tapioca::unbounded;

    // ── Layout-specific enums (tapiru only) ─────────────────────────────────

    /** @brief How text overflows its container. */
    enum class overflow_mode : uint8_t {
        wrap = 0,
        truncate = 1,
        ellipsis = 2,
    };

    /** @brief Text horizontal alignment / content justification. */
    enum class justify : uint8_t {
        left = 0,
        center = 1,
        right = 2,
        space_between = 3,
        space_around = 4,
    };

    /** @brief Cross-axis alignment for flex containers. */
    enum class align_items : uint8_t {
        stretch = 0,
        start = 1,
        center = 2,
        end = 3,
    };

    /** @brief Direction for gauge widgets. */
    enum class gauge_direction : uint8_t {
        horizontal = 0,
        vertical = 1,
    };

} // namespace tapiru
