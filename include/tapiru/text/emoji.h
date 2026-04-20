#pragma once

/**
 * @file emoji.h
 * @brief Emoji shortcode lookup table.
 *
 * Resolves :shortcode: syntax to Unicode emoji codepoints.
 *
 * Usage:
 *   auto cp = tapiru::emoji_lookup(":fire:");       // → U+1F525
 *   auto s  = tapiru::replace_emoji(":wave: Hi!");  // → "👋 Hi!"
 */

#include <cstdint>
#include <string>
#include <string_view>

#include "tapiru/exports.h"

namespace tapiru {

/**
 * @brief Look up an emoji shortcode (with colons).
 * @param shortcode  e.g. ":fire:", ":wave:", ":heart:"
 * @return the Unicode codepoint, or 0 if not found
 */
[[nodiscard]] TAPIRU_API char32_t emoji_lookup(std::string_view shortcode) noexcept;

/**
 * @brief Replace all :shortcode: occurrences in a string with their emoji.
 * @param input  the input string
 * @return the string with shortcodes replaced by UTF-8 emoji
 */
[[nodiscard]] TAPIRU_API std::string replace_emoji(std::string_view input);

}  // namespace tapiru
