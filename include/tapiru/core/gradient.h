#pragma once

/**
 * @file gradient.h
 * @brief Compatibility header -- gradient types moved to tapioca/color.h.
 */

#include "tapioca/color.h"

namespace tapiru {

    using tapioca::gradient_direction;
    using tapioca::linear_gradient;
    using tapioca::resolve_gradient;

    // Extended color utilities also available via tapioca::
    using tapioca::blend_oklch;
    using tapioca::contrast_ratio;
    using tapioca::hsl;
    using tapioca::hsl_to_rgb;
    using tapioca::oklch;
    using tapioca::oklch_to_rgb;
    using tapioca::relative_luminance;
    using tapioca::rgb_to_hsl;
    using tapioca::rgb_to_oklch;

} // namespace tapiru
