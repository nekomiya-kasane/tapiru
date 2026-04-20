#pragma once

/**
 * @file unicode_width.h
 * @brief Compatibility header -- types moved to tapioca.
 *
 * Includes tapioca/unicode_width.h and re-exports all symbols into tapiru::.
 */

#include "tapioca/unicode_width.h"

namespace tapiru {

using tapioca::char_width;
using tapioca::string_width;
using tapioca::utf8_decode;

}  // namespace tapiru
