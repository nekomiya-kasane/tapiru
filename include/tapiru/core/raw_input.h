#pragma once

/**
 * @file raw_input.h
 * @brief Compatibility header -- types moved to tapioca.
 *
 * Includes tapioca/raw_input.h and re-exports all symbols into tapiru::.
 */

#include "tapioca/raw_input.h"

namespace tapiru {

using tapioca::raw_mode;
using tapioca::poll_event;

}  // namespace tapiru
