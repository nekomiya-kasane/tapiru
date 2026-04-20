#pragma once

/**
 * @file paragraph.h
 * @brief Word-wrapping paragraph element factories.
 */

#include "tapiru/core/element.h"
#include "tapiru/exports.h"

#include <string_view>

namespace tapiru {

TAPIRU_API element make_paragraph(std::string_view text);
TAPIRU_API element make_paragraph_justify(std::string_view text);

} // namespace tapiru
