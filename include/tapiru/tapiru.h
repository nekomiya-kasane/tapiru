#pragma once

/**
 * @file tapiru.h
 * @brief Umbrella header for the tapiru terminal & TUI library.
 */

#include "tapiru/exports.h"

namespace tapiru {

/** @brief Returns the library version as a string literal. */
[[nodiscard]] TAPIRU_API const char* version() noexcept;

}  // namespace tapiru
