#pragma once

/**
 * @file markup.h
 * @brief Rich-like markup parser: "[bold red]Hello[/bold red]World" → fragments.
 *
 * Syntax:
 *   [tag]text[/tag]   — open/close a style tag
 *   [/]               — close all open tags (reset to default)
 *   [tag1 tag2]       — space-separated compound tags
 *   [[                — escaped literal '['
 *
 * Supported tags:
 *   Colors (fg):  black, red, green, yellow, blue, magenta, cyan, white,
 *                 bright_black, bright_red, ... bright_white
 *   Colors (bg):  on_black, on_red, ... on_bright_white
 *   Attributes:   bold, dim, italic, underline, blink, reverse, hidden, strike
 *   RGB:          #RRGGBB (fg), on_#RRGGBB (bg)
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"

#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

    /**
     * @brief A fragment of text with an associated style.
     */
    struct text_fragment {
        std::string text;
        style sty;
    };

    /**
     * @brief Parse a markup string into styled text fragments.
     *
     * @param markup  the input string with [tag] markup
     * @return vector of text_fragment, each with its resolved style
     */
    [[nodiscard]] TAPIRU_API std::vector<text_fragment> parse_markup(std::string_view markup);

    /**
     * @brief Strip all markup tags from a string, returning plain text.
     */
    [[nodiscard]] TAPIRU_API std::string strip_markup(std::string_view markup);

} // namespace tapiru
