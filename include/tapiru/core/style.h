#pragma once

/**
 * @file style.h
 * @brief Compatibility header -- types moved to tapioca.
 *
 * Includes tapioca/style.h and re-exports all types into tapiru::.
 */

#include "tapioca/style.h"

namespace tapiru {

using tapioca::color_kind;
using tapioca::color;
using tapioca::attr;
using tapioca::has_attr;
using tapioca::style;

namespace colors = tapioca::colors;

}  // namespace tapiru
