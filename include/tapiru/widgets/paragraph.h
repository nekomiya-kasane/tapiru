#pragma once

/**
 * @file paragraph.h
 * @brief Word-wrapping paragraph element factories.
 */

#include <string_view>

#include "tapiru/core/element.h"
#include "tapiru/exports.h"

namespace tapiru {

TAPIRU_API element make_paragraph(std::string_view text);
TAPIRU_API element make_paragraph_justify(std::string_view text);

}  // namespace tapiru
