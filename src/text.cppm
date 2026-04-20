/**
 * @file text.cppm
 * @brief C++20 module partition for tapiru text utilities.
 *
 * Exports: markup, emoji, markdown.
 */

export module tapiru:text;

export {
#include "tapiru/text/markup.h"
#include "tapiru/text/emoji.h"
#include "tapiru/text/markdown.h"
}
