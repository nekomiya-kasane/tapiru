/**
 * @file core.cppm
 * @brief C++20 module partition for tapiru core types.
 *
 * Exports: element, component, screen, style, color, terminal, input, raw_input, live, logging.
 */

export module tapiru:core;

export {
#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"
#include "tapiru/core/input.h"
#include "tapiru/core/raw_input.h"
#include "tapiru/core/element.h"
#include "tapiru/core/decorator.h"
#include "tapiru/core/component.h"
#include "tapiru/core/screen.h"
#include "tapiru/core/live.h"
#include "tapiru/core/logging.h"
#include "tapiru/core/console.h"
}
