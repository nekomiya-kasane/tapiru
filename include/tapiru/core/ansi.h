#pragma once

/**
 * @file ansi.h
 * @brief Compatibility header -- types moved to tapioca.
 *
 * Includes tapioca/ansi_emitter.h and re-exports all symbols into tapiru::.
 */

#include "tapioca/ansi_emitter.h"
#include "tapiru/core/style.h"

namespace tapiru {

    namespace ansi = tapioca::ansi;

    using tapioca::ansi_emitter;

} // namespace tapiru
