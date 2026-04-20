#pragma once

/**
 * @file gauge.h
 * @brief Progress gauge element factories.
 *
 * Renders a horizontal or vertical progress bar using Unicode block characters.
 */

#include <cstdint>

#include "tapiru/core/element.h"
#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

namespace tapiru {

TAPIRU_API element make_gauge(float progress);
TAPIRU_API element make_gauge(float progress, const style& filled, const style& remaining);
TAPIRU_API element make_gauge_direction(float progress, gauge_direction dir);

}  // namespace tapiru
