#pragma once

/**
 * @file decorator.h
 * @brief Predefined decorator factories for element pipe composition.
 *
 * Usage:
 *   auto styled = element(text_builder("hello")) | border() | bold() | fg_color(colors::cyan);
 */

#include <cstdint>
#include <string_view>

#include "tapiru/core/element.h"
#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

namespace tapiru {

// ── Layout decorators ───────────────────────────────────────────────────

TAPIRU_API decorator border(border_style bs = border_style::rounded);
TAPIRU_API decorator padding(uint8_t all);
TAPIRU_API decorator padding(uint8_t v, uint8_t h);
TAPIRU_API decorator padding(uint8_t top, uint8_t right, uint8_t bottom, uint8_t left);
TAPIRU_API decorator size(uint32_t w, uint32_t h);
TAPIRU_API decorator min_size(uint32_t w, uint32_t h);
TAPIRU_API decorator max_size(uint32_t w, uint32_t h);
TAPIRU_API decorator flex(uint32_t ratio = 1);
TAPIRU_API decorator center();

// ── Style decorators ────────────────────────────────────────────────────

TAPIRU_API decorator fg_color(tapiru::color c);
TAPIRU_API decorator bg_color(tapiru::color c);
TAPIRU_API decorator bold();
TAPIRU_API decorator dim();
TAPIRU_API decorator italic();
TAPIRU_API decorator underline();

// ── Visual effect decorators ────────────────────────────────────────────

TAPIRU_API decorator focus_indicator();

// ── Conditional decorators ──────────────────────────────────────────────

TAPIRU_API decorator maybe(const bool* show);

}  // namespace tapiru
