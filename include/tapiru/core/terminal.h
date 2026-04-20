#pragma once

/**
 * @file terminal.h
 * @brief Compatibility header -- types moved to tapioca.
 *
 * Includes tapioca/terminal.h and re-exports all types into tapiru::.
 */

#include "tapioca/terminal.h"

namespace tapiru {

using tapioca::terminal_size;
using tapioca::color_depth;
using tapioca::terminal_caps;

namespace terminal = tapioca::terminal;

}  // namespace tapiru
