#pragma once

/**
 * @file constexpr_markup.h
 * @brief Compile-time markup tag resolution and style construction.
 *
 * Provides consteval/constexpr utilities to resolve markup tags to styles
 * at compile time, avoiding runtime parsing overhead for string literals.
 *
 * Usage:
 *   constexpr auto s = tapiru::markup_style("bold red on_blue");
 *   static_assert(s.fg == tapiru::colors::red);
 *   static_assert(tapiru::has_attr(s.attrs, tapiru::attr::bold));
 *
 *   // Full compile-time markup parsing:
 *   constexpr auto m = tapiru::ct_parse_markup("[bold]Hello[/] World");
 *   static_assert(m.count == 2);
 *   static_assert(m.fragments[0].sty.attrs == tapiru::attr::bold);
 */

#include "tapiru/core/style.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace tapiru {

    // ── Constexpr tag resolution ────────────────────────────────────────────

    namespace detail {

        constexpr bool sv_eq(const char *a, size_t alen, const char *b, size_t blen) {
            if (alen != blen) {
                return false;
            }
            for (size_t i = 0; i < alen; ++i) {
                if (a[i] != b[i]) {
                    return false;
                }
            }
            return true;
        }

        constexpr uint8_t hex_digit(char c) {
            if (c >= '0' && c <= '9') {
                return static_cast<uint8_t>(c - '0');
            }
            if (c >= 'a' && c <= 'f') {
                return static_cast<uint8_t>(c - 'a' + 10);
            }
            if (c >= 'A' && c <= 'F') {
                return static_cast<uint8_t>(c - 'A' + 10);
            }
            return 0xFF;
        }

        constexpr bool try_parse_hex_color_ct(const char *s, size_t len, color &out) {
            if (len != 7 || s[0] != '#') {
                return false;
            }
            uint8_t r1 = hex_digit(s[1]), r2 = hex_digit(s[2]);
            uint8_t g1 = hex_digit(s[3]), g2 = hex_digit(s[4]);
            uint8_t b1 = hex_digit(s[5]), b2 = hex_digit(s[6]);
            if (r1 == 0xFF || r2 == 0xFF || g1 == 0xFF || g2 == 0xFF || b1 == 0xFF || b2 == 0xFF) {
                return false;
            }
            out = color::from_rgb(static_cast<uint8_t>(r1 * 16 + r2), static_cast<uint8_t>(g1 * 16 + g2),
                                  static_cast<uint8_t>(b1 * 16 + b2));
            return true;
        }

        // Constexpr decimal uint8_t parser: parses digits starting at pos, returns value and advances pos.
        constexpr uint8_t ct_parse_uint8(const char *s, size_t len, size_t &pos) {
            uint16_t val = 0;
            while (pos < len && s[pos] >= '0' && s[pos] <= '9') {
                val = val * 10 + static_cast<uint16_t>(s[pos] - '0');
                ++pos;
            }
            return static_cast<uint8_t>(val > 255 ? 255 : val);
        }

        // Try parse rgb(R,G,B) or on_rgb(R,G,B) at compile time
        constexpr bool try_parse_rgb_func_ct(const char *tag, size_t len, color &out, bool is_bg) {
            // Expected: "rgb(R,G,B)" or "on_rgb(R,G,B)"
            size_t prefix = is_bg ? 7 : 4; // "on_rgb(" vs "rgb("
            if (len < prefix + 5) {
                return false; // minimum "rgb(0,0,0)" = 10 chars
            }
            // Check prefix
            if (is_bg) {
                if (tag[0] != 'o' || tag[1] != 'n' || tag[2] != '_' || tag[3] != 'r' || tag[4] != 'g' ||
                    tag[5] != 'b' || tag[6] != '(') {
                    return false;
                }
            } else {
                if (tag[0] != 'r' || tag[1] != 'g' || tag[2] != 'b' || tag[3] != '(') {
                    return false;
                }
            }
            // Check closing paren
            if (tag[len - 1] != ')') {
                return false;
            }
            size_t pos = prefix;
            uint8_t r = ct_parse_uint8(tag, len - 1, pos);
            if (pos >= len - 1 || tag[pos] != ',') {
                return false;
            }
            ++pos;
            uint8_t g = ct_parse_uint8(tag, len - 1, pos);
            if (pos >= len - 1 || tag[pos] != ',') {
                return false;
            }
            ++pos;
            uint8_t b = ct_parse_uint8(tag, len - 1, pos);
            if (pos != len - 1) {
                return false;
            }
            out = color::from_rgb(r, g, b);
            return true;
        }

        // Try parse color256(N) or on_color256(N) at compile time
        constexpr bool try_parse_color256_ct(const char *tag, size_t len, color &out, bool is_bg) {
            // Expected: "color256(N)" or "on_color256(N)"
            size_t prefix = is_bg ? 12 : 9; // "on_color256(" vs "color256("
            if (len < prefix + 2) {
                return false; // minimum "color256(0)" = 11 chars
            }
            if (is_bg) {
                if (!sv_eq(tag, 12, "on_color256(", 12)) {
                    return false;
                }
            } else {
                if (!sv_eq(tag, 9, "color256(", 9)) {
                    return false;
                }
            }
            if (tag[len - 1] != ')') {
                return false;
            }
            size_t pos = prefix;
            uint8_t idx = ct_parse_uint8(tag, len - 1, pos);
            if (pos != len - 1) {
                return false;
            }
            out = color::from_index_256(idx);
            return true;
        }

#define TAPIRU_CT_TAG(name, action)                                                                                    \
    if (sv_eq(tag, len, name, sizeof(name) - 1)) {                                                                     \
        action;                                                                                                        \
        return true;                                                                                                   \
    }

        constexpr bool apply_tag_ct(const char *tag, size_t len, style &s) {
            // Attributes
            TAPIRU_CT_TAG("bold", s.attrs |= attr::bold)
            TAPIRU_CT_TAG("dim", s.attrs |= attr::dim)
            TAPIRU_CT_TAG("italic", s.attrs |= attr::italic)
            TAPIRU_CT_TAG("underline", s.attrs |= attr::underline)
            TAPIRU_CT_TAG("blink", s.attrs |= attr::blink)
            TAPIRU_CT_TAG("reverse", s.attrs |= attr::reverse)
            TAPIRU_CT_TAG("hidden", s.attrs |= attr::hidden)
            TAPIRU_CT_TAG("strike", s.attrs |= attr::strike)
            TAPIRU_CT_TAG("double_underline", s.attrs |= attr::double_underline)
            TAPIRU_CT_TAG("overline", s.attrs |= attr::overline)
            TAPIRU_CT_TAG("superscript", s.attrs |= attr::superscript)
            TAPIRU_CT_TAG("subscript", s.attrs |= attr::subscript)

            // Negation tags
            TAPIRU_CT_TAG("not_bold", s.attrs = s.attrs & ~attr::bold)
            TAPIRU_CT_TAG("not_dim", s.attrs = s.attrs & ~attr::dim)
            TAPIRU_CT_TAG("not_italic", s.attrs = s.attrs & ~attr::italic)
            TAPIRU_CT_TAG("not_underline", s.attrs = s.attrs & ~attr::underline)
            TAPIRU_CT_TAG("not_blink", s.attrs = s.attrs & ~attr::blink)
            TAPIRU_CT_TAG("not_reverse", s.attrs = s.attrs & ~attr::reverse)
            TAPIRU_CT_TAG("not_hidden", s.attrs = s.attrs & ~attr::hidden)
            TAPIRU_CT_TAG("not_strike", s.attrs = s.attrs & ~attr::strike)
            TAPIRU_CT_TAG("not_overline", s.attrs = s.attrs & ~attr::overline)

            // Color reset
            TAPIRU_CT_TAG("default", s.fg = color::default_color())
            TAPIRU_CT_TAG("on_default", s.bg = color::default_color())

            // Semantic aliases
            TAPIRU_CT_TAG("error", (s.attrs |= attr::bold, s.fg = colors::red))
            TAPIRU_CT_TAG("warning", (s.attrs |= attr::bold, s.fg = colors::yellow))
            TAPIRU_CT_TAG("success", (s.attrs |= attr::bold, s.fg = colors::green))
            TAPIRU_CT_TAG("info", (s.attrs |= attr::bold, s.fg = colors::cyan))
            TAPIRU_CT_TAG("debug", s.attrs |= attr::dim)
            TAPIRU_CT_TAG("muted", s.attrs |= attr::dim)
            TAPIRU_CT_TAG("highlight", (s.attrs |= attr::bold, s.fg = colors::black, s.bg = colors::yellow))
            TAPIRU_CT_TAG("code_inline",
                          (s.attrs |= attr::bold, s.fg = colors::cyan, s.bg = color::from_rgb(0x1e, 0x1e, 0x2e)))
            TAPIRU_CT_TAG("url", (s.attrs |= attr::underline, s.fg = colors::bright_blue))
            TAPIRU_CT_TAG("path", (s.attrs |= attr::italic, s.fg = colors::bright_green))
            TAPIRU_CT_TAG("key",
                          (s.attrs |= attr::bold, s.fg = colors::white, s.bg = color::from_rgb(0x3a, 0x3a, 0x4a)))
            TAPIRU_CT_TAG("badge.red", (s.attrs |= attr::bold, s.fg = colors::white, s.bg = colors::red))
            TAPIRU_CT_TAG("badge.green", (s.attrs |= attr::bold, s.fg = colors::white, s.bg = colors::green))
            TAPIRU_CT_TAG("badge.blue", (s.attrs |= attr::bold, s.fg = colors::white, s.bg = colors::blue))
            TAPIRU_CT_TAG("badge.yellow", (s.attrs |= attr::bold, s.fg = colors::black, s.bg = colors::yellow))

            // Foreground colors
            TAPIRU_CT_TAG("black", s.fg = colors::black)
            TAPIRU_CT_TAG("red", s.fg = colors::red)
            TAPIRU_CT_TAG("green", s.fg = colors::green)
            TAPIRU_CT_TAG("yellow", s.fg = colors::yellow)
            TAPIRU_CT_TAG("blue", s.fg = colors::blue)
            TAPIRU_CT_TAG("magenta", s.fg = colors::magenta)
            TAPIRU_CT_TAG("cyan", s.fg = colors::cyan)
            TAPIRU_CT_TAG("white", s.fg = colors::white)
            TAPIRU_CT_TAG("bright_black", s.fg = colors::bright_black)
            TAPIRU_CT_TAG("bright_red", s.fg = colors::bright_red)
            TAPIRU_CT_TAG("bright_green", s.fg = colors::bright_green)
            TAPIRU_CT_TAG("bright_yellow", s.fg = colors::bright_yellow)
            TAPIRU_CT_TAG("bright_blue", s.fg = colors::bright_blue)
            TAPIRU_CT_TAG("bright_magenta", s.fg = colors::bright_magenta)
            TAPIRU_CT_TAG("bright_cyan", s.fg = colors::bright_cyan)
            TAPIRU_CT_TAG("bright_white", s.fg = colors::bright_white)

            // Background colors
            TAPIRU_CT_TAG("on_black", s.bg = colors::black)
            TAPIRU_CT_TAG("on_red", s.bg = colors::red)
            TAPIRU_CT_TAG("on_green", s.bg = colors::green)
            TAPIRU_CT_TAG("on_yellow", s.bg = colors::yellow)
            TAPIRU_CT_TAG("on_blue", s.bg = colors::blue)
            TAPIRU_CT_TAG("on_magenta", s.bg = colors::magenta)
            TAPIRU_CT_TAG("on_cyan", s.bg = colors::cyan)
            TAPIRU_CT_TAG("on_white", s.bg = colors::white)
            TAPIRU_CT_TAG("on_bright_black", s.bg = colors::bright_black)
            TAPIRU_CT_TAG("on_bright_red", s.bg = colors::bright_red)
            TAPIRU_CT_TAG("on_bright_green", s.bg = colors::bright_green)
            TAPIRU_CT_TAG("on_bright_yellow", s.bg = colors::bright_yellow)
            TAPIRU_CT_TAG("on_bright_blue", s.bg = colors::bright_blue)
            TAPIRU_CT_TAG("on_bright_magenta", s.bg = colors::bright_magenta)
            TAPIRU_CT_TAG("on_bright_cyan", s.bg = colors::bright_cyan)
            TAPIRU_CT_TAG("on_bright_white", s.bg = colors::bright_white)

            // RGB hex: #RRGGBB (fg)
            if (len == 7 && tag[0] == '#') {
                color c;
                if (try_parse_hex_color_ct(tag, len, c)) {
                    s.fg = c;
                    return true;
                }
            }

            // RGB hex: on_#RRGGBB (bg)
            if (len == 10 && tag[0] == 'o' && tag[1] == 'n' && tag[2] == '_' && tag[3] == '#') {
                color c;
                if (try_parse_hex_color_ct(tag + 3, len - 3, c)) {
                    s.bg = c;
                    return true;
                }
            }

            // rgb(R,G,B) (fg)
            if (len >= 10 && tag[0] == 'r' && tag[1] == 'g' && tag[2] == 'b' && tag[3] == '(') {
                color c;
                if (try_parse_rgb_func_ct(tag, len, c, false)) {
                    s.fg = c;
                    return true;
                }
            }

            // on_rgb(R,G,B) (bg)
            if (len >= 13 && tag[0] == 'o' && tag[1] == 'n' && tag[2] == '_' && tag[3] == 'r' && tag[4] == 'g' &&
                tag[5] == 'b' && tag[6] == '(') {
                color c;
                if (try_parse_rgb_func_ct(tag, len, c, true)) {
                    s.bg = c;
                    return true;
                }
            }

            // color256(N) (fg)
            if (len >= 11 && tag[0] == 'c') {
                color c;
                if (try_parse_color256_ct(tag, len, c, false)) {
                    s.fg = c;
                    return true;
                }
            }

            // on_color256(N) (bg)
            if (len >= 14 && tag[0] == 'o' && tag[1] == 'n' && tag[2] == '_' && tag[3] == 'c') {
                color c;
                if (try_parse_color256_ct(tag, len, c, true)) {
                    s.bg = c;
                    return true;
                }
            }

            return false;
        }

#undef TAPIRU_CT_TAG

        constexpr void apply_compound_tag_ct(const char *tag, size_t len, style &s) {
            size_t pos = 0;
            while (pos < len) {
                while (pos < len && tag[pos] == ' ') {
                    ++pos;
                }
                if (pos >= len) {
                    break;
                }
                size_t end = pos;
                while (end < len && tag[end] != ' ') {
                    ++end;
                }
                apply_tag_ct(tag + pos, end - pos, s);
                pos = end;
            }
        }

    } // namespace detail

    // ── Public API: markup_style ────────────────────────────────────────────

    /**
     * @brief Resolve a compound tag string to a style at compile time.
     *
     * @param tag  space-separated tag string, e.g. "bold red on_blue"
     * @return the resolved style
     */
    template <size_t N> consteval style markup_style(const char (&tag)[N]) {
        style s{};
        detail::apply_compound_tag_ct(tag, N - 1, s);
        return s;
    }

    // ── Full compile-time markup parser ─────────────────────────────────────

    /**
     * @brief A compile-time text fragment: offset+length into the original string + style.
     */
    struct ct_fragment {
        size_t offset = 0;
        size_t length = 0;
        style sty{};
    };

    /**
     * @brief Result of compile-time markup parsing.
     * @tparam MaxFragments maximum number of fragments supported
     */
    template <size_t MaxFragments = 16> struct static_markup {
        std::array<ct_fragment, MaxFragments> fragments{};
        size_t count = 0;
    };

    namespace detail {

        template <size_t N, size_t MaxFragments>
        consteval static_markup<MaxFragments> ct_parse_impl(const char (&markup)[N]) {
            static_markup<MaxFragments> result{};
            constexpr size_t len = N - 1;

            style style_stack[32]{};
            size_t stack_depth = 1; // style_stack[0] = default

            size_t text_start = 0;
            bool in_text = false;
            size_t i = 0;

            auto flush_text = [&](size_t end) {
                if (in_text && end > text_start && result.count < MaxFragments) {
                    result.fragments[result.count].offset = text_start;
                    result.fragments[result.count].length = end - text_start;
                    result.fragments[result.count].sty = style_stack[stack_depth - 1];
                    result.count++;
                    in_text = false;
                }
            };

            while (i < len) {
                if (markup[i] == '[') {
                    // Escaped [[
                    if (i + 1 < len && markup[i + 1] == '[') {
                        if (!in_text) {
                            text_start = i;
                            in_text = true;
                        }
                        // We include the first [ as literal text, skip the second
                        // But since we can't modify the string, we flush up to i,
                        // then record i..i+1 as a single [ character
                        // This is tricky in constexpr — just include both and let
                        // runtime strip the extra [. For constexpr, treat [[ as
                        // a 2-char sequence that represents [.
                        i += 2;
                        continue;
                    }

                    // Find closing ]
                    size_t close = i + 1;
                    while (close < len && markup[close] != ']') {
                        ++close;
                    }
                    if (close >= len) {
                        // No closing ] — treat as literal
                        if (!in_text) {
                            text_start = i;
                            in_text = true;
                        }
                        ++i;
                        continue;
                    }

                    // Flush current text
                    flush_text(i);

                    // Parse tag content
                    size_t tag_start = i + 1;
                    size_t tag_len = close - tag_start;

                    if (tag_len == 1 && markup[tag_start] == '/') {
                        // [/] — reset
                        stack_depth = 1;
                        style_stack[0] = style{};
                    } else if (tag_len > 0 && markup[tag_start] == '/') {
                        // [/tag] — pop
                        if (stack_depth > 1) {
                            stack_depth--;
                        }
                    } else {
                        // [tag] — push
                        if (stack_depth < 32) {
                            style_stack[stack_depth] = style_stack[stack_depth - 1];
                            apply_compound_tag_ct(markup + tag_start, tag_len, style_stack[stack_depth]);
                            stack_depth++;
                        }
                    }

                    i = close + 1;
                } else {
                    if (!in_text) {
                        text_start = i;
                        in_text = true;
                    }
                    ++i;
                }
            }

            flush_text(len);
            return result;
        }

    } // namespace detail

    /**
     * @brief Parse a markup string literal at compile time.
     *
     * Returns a static_markup with up to MaxFragments fragments.
     * Each fragment stores offset/length into the original string + resolved style.
     *
     * @tparam MaxFragments max fragments (default 16)
     * @param markup the string literal
     */
    template <size_t MaxFragments = 16, size_t N>
    consteval static_markup<MaxFragments> ct_parse_markup(const char (&markup)[N]) {
        return detail::ct_parse_impl<N, MaxFragments>(markup);
    }

    // ── Extended markup: block-level descriptors ─────────────────────────

    /**
     * @brief Kind of block-level operation.
     */
    enum class block_kind : uint8_t {
        none = 0,      ///< Plain inline text (existing behavior)
        box_open,      ///< Open a bordered box
        box_close,     ///< Close a bordered box
        rule,          ///< Horizontal rule
        indent_open,   ///< Open indented block
        indent_close,  ///< Close indented block
        align_open,    ///< Open alignment block (center/right)
        align_close,   ///< Close alignment block
        quote_open,    ///< Open blockquote
        quote_close,   ///< Close blockquote
        code_open,     ///< Open code block
        code_close,    ///< Close code block
        heading,       ///< Heading (h1/h2/h3) — style push
        heading_close, ///< Close heading — style pop
        list_open,     ///< Open list (ul/ol)
        list_close,    ///< Close list
        list_item,     ///< List item bullet/number
        pad_open,      ///< Open padded block
        pad_close,     ///< Close padded block
        progress_bar,  ///< Inline progress bar
        fraction_bar,  ///< Fraction bar (N/M)
        line_break,    ///< Explicit line break
        spacing,       ///< N spaces
        emoji,         ///< Emoji (name stored as offset+len, resolved at runtime)
        link_open,     ///< OSC 8 hyperlink open
        link_close,    ///< OSC 8 hyperlink close
        columns_open,  ///< Open columns
        columns_close, ///< Close columns
    };

    /**
     * @brief Block-level parameters for a rich fragment.
     *
     * All fields are constexpr-safe POD. Strings are stored as offset+length
     * into the source markup literal.
     */
    struct ct_block_params {
        block_kind kind = block_kind::none;
        uint8_t border = 2;        ///< border_style as uint8_t (default: rounded=2)
        uint8_t level = 0;         ///< heading level (1-3), indent spaces, pad amount
        uint8_t numerator = 0;     ///< progress percent or bar numerator
        uint8_t denominator = 0;   ///< bar denominator
        uint8_t align = 0;         ///< justify as uint8_t (0=left, 1=center, 2=right)
        bool ordered = false;      ///< list ordered?
        uint16_t title_offset = 0; ///< offset into source for rule/box title
        uint16_t title_length = 0; ///< length of title
        uint16_t url_offset = 0;   ///< offset into source for link URL
        uint16_t url_length = 0;   ///< length of URL
        uint16_t name_offset = 0;  ///< offset into source for emoji name
        uint16_t name_length = 0;  ///< length of emoji name
        style content_sty{};       ///< style for heading/code content
    };

    /**
     * @brief Extended compile-time fragment with block descriptor.
     *
     * When block.kind == none, this is a plain inline text fragment (backward compatible).
     */
    struct ct_rich_fragment {
        size_t offset = 0;
        size_t length = 0;
        style sty{};
        ct_block_params block{};
    };

    /**
     * @brief Result of compile-time rich markup parsing.
     */
    template <size_t MaxFragments = 64> struct static_rich_markup {
        std::array<ct_rich_fragment, MaxFragments> fragments{};
        size_t count = 0;
    };

    namespace detail {

        constexpr bool sv_starts_with(const char *s, size_t slen, const char *prefix, size_t plen) {
            if (slen < plen) {
                return false;
            }
            for (size_t i = 0; i < plen; ++i) {
                if (s[i] != prefix[i]) {
                    return false;
                }
            }
            return true;
        }

        // Try to recognize a block tag. Returns true if recognized.
        constexpr bool try_block_tag_ct(const char *tag, size_t len, [[maybe_unused]] const char *markup,
                                        size_t tag_abs_offset, ct_rich_fragment &frag) {
            // [br]
            if (sv_eq(tag, len, "br", 2)) {
                frag.block.kind = block_kind::line_break;
                return true;
            }
            // [hr] — alias for rule
            if (sv_eq(tag, len, "hr", 2)) {
                frag.block.kind = block_kind::rule;
                return true;
            }
            // [rule] or [rule=Title]
            if (sv_eq(tag, len, "rule", 4)) {
                frag.block.kind = block_kind::rule;
                return true;
            }
            if (len > 5 && sv_starts_with(tag, len, "rule=", 5)) {
                frag.block.kind = block_kind::rule;
                frag.block.title_offset = static_cast<uint16_t>(tag_abs_offset + 5);
                frag.block.title_length = static_cast<uint16_t>(len - 5);
                return true;
            }
            // [sp=N]
            if (len > 3 && sv_starts_with(tag, len, "sp=", 3)) {
                frag.block.kind = block_kind::spacing;
                size_t pos = 3;
                frag.block.level = ct_parse_uint8(tag, len, pos);
                return true;
            }
            // [box], [box.heavy], [box.double], [box.ascii], [box title="T"]
            if (sv_eq(tag, len, "box", 3)) {
                frag.block.kind = block_kind::box_open;
                frag.block.border = 2; // rounded
                return true;
            }
            if (sv_eq(tag, len, "box.heavy", 9)) {
                frag.block.kind = block_kind::box_open;
                frag.block.border = 3; // heavy
                return true;
            }
            if (sv_eq(tag, len, "box.double", 10)) {
                frag.block.kind = block_kind::box_open;
                frag.block.border = 4; // double_
                return true;
            }
            if (sv_eq(tag, len, "box.ascii", 9)) {
                frag.block.kind = block_kind::box_open;
                frag.block.border = 1; // ascii
                return true;
            }
            // [box title="T"] — find title="..." inside tag
            if (len > 4 && sv_starts_with(tag, len, "box ", 4)) {
                frag.block.kind = block_kind::box_open;
                frag.block.border = 2;
                // Look for title="..."
                for (size_t i = 4; i + 7 < len; ++i) {
                    if (sv_starts_with(tag + i, len - i, "title=\"", 7)) {
                        size_t start = i + 7;
                        size_t end = start;
                        while (end < len && tag[end] != '"') {
                            ++end;
                        }
                        frag.block.title_offset = static_cast<uint16_t>(tag_abs_offset + start);
                        frag.block.title_length = static_cast<uint16_t>(end - start);
                        break;
                    }
                }
                return true;
            }
            // [indent] or [indent=N]
            if (sv_eq(tag, len, "indent", 6)) {
                frag.block.kind = block_kind::indent_open;
                frag.block.level = 4;
                return true;
            }
            if (len > 7 && sv_starts_with(tag, len, "indent=", 7)) {
                frag.block.kind = block_kind::indent_open;
                size_t pos = 7;
                frag.block.level = ct_parse_uint8(tag, len, pos);
                return true;
            }
            // [center] / [right]
            if (sv_eq(tag, len, "center", 6)) {
                frag.block.kind = block_kind::align_open;
                frag.block.align = 1;
                return true;
            }
            if (sv_eq(tag, len, "right", 5)) {
                frag.block.kind = block_kind::align_open;
                frag.block.align = 2;
                return true;
            }
            // [quote]
            if (sv_eq(tag, len, "quote", 5)) {
                frag.block.kind = block_kind::quote_open;
                return true;
            }
            // [code]
            if (sv_eq(tag, len, "code", 4)) {
                frag.block.kind = block_kind::code_open;
                return true;
            }
            // [h1] / [h2] / [h3]
            if (sv_eq(tag, len, "h1", 2)) {
                frag.block.kind = block_kind::heading;
                frag.block.level = 1;
                frag.block.content_sty = style{{}, {}, attr::bold | attr::underline};
                return true;
            }
            if (sv_eq(tag, len, "h2", 2)) {
                frag.block.kind = block_kind::heading;
                frag.block.level = 2;
                frag.block.content_sty = style{{}, {}, attr::bold};
                return true;
            }
            if (sv_eq(tag, len, "h3", 2)) {
                frag.block.kind = block_kind::heading;
                frag.block.level = 3;
                frag.block.content_sty = style{{}, {}, attr::italic};
                return true;
            }
            // [ul] / [ol]
            if (sv_eq(tag, len, "ul", 2)) {
                frag.block.kind = block_kind::list_open;
                frag.block.ordered = false;
                return true;
            }
            if (sv_eq(tag, len, "ol", 2)) {
                frag.block.kind = block_kind::list_open;
                frag.block.ordered = true;
                return true;
            }
            // [columns]
            if (sv_eq(tag, len, "columns", 7)) {
                frag.block.kind = block_kind::columns_open;
                return true;
            }
            // [pad=N]
            if (len > 4 && sv_starts_with(tag, len, "pad=", 4)) {
                frag.block.kind = block_kind::pad_open;
                size_t pos = 4;
                frag.block.level = ct_parse_uint8(tag, len, pos);
                return true;
            }
            // [progress=P]
            if (len > 9 && sv_starts_with(tag, len, "progress=", 9)) {
                frag.block.kind = block_kind::progress_bar;
                size_t pos = 9;
                frag.block.numerator = ct_parse_uint8(tag, len, pos);
                return true;
            }
            // [bar=N/M]
            if (len > 4 && sv_starts_with(tag, len, "bar=", 4)) {
                frag.block.kind = block_kind::fraction_bar;
                size_t pos = 4;
                frag.block.numerator = ct_parse_uint8(tag, len, pos);
                if (pos < len && tag[pos] == '/') {
                    ++pos;
                    frag.block.denominator = ct_parse_uint8(tag, len, pos);
                }
                return true;
            }
            // [emoji=NAME]
            if (len > 6 && sv_starts_with(tag, len, "emoji=", 6)) {
                frag.block.kind = block_kind::emoji;
                frag.block.name_offset = static_cast<uint16_t>(tag_abs_offset + 6);
                frag.block.name_length = static_cast<uint16_t>(len - 6);
                return true;
            }
            // [link=URL]
            if (len > 5 && sv_starts_with(tag, len, "link=", 5)) {
                frag.block.kind = block_kind::link_open;
                frag.block.url_offset = static_cast<uint16_t>(tag_abs_offset + 5);
                frag.block.url_length = static_cast<uint16_t>(len - 5);
                return true;
            }

            return false;
        }

        // Map closing tag name to block_kind
        constexpr block_kind closing_block_kind(const char *tag, size_t len) {
            // tag starts after '/', e.g. "box", "indent", etc.
            if (sv_eq(tag, len, "box", 3) || sv_starts_with(tag, len, "box.", 4) ||
                sv_starts_with(tag, len, "box ", 4)) {
                return block_kind::box_close;
            }
            if (sv_eq(tag, len, "indent", 6) || sv_starts_with(tag, len, "indent=", 7)) {
                return block_kind::indent_close;
            }
            if (sv_eq(tag, len, "center", 6) || sv_eq(tag, len, "right", 5)) {
                return block_kind::align_close;
            }
            if (sv_eq(tag, len, "quote", 5)) {
                return block_kind::quote_close;
            }
            if (sv_eq(tag, len, "code", 4)) {
                return block_kind::code_close;
            }
            if (sv_eq(tag, len, "h1", 2) || sv_eq(tag, len, "h2", 2) || sv_eq(tag, len, "h3", 2)) {
                return block_kind::heading_close;
            }
            if (sv_eq(tag, len, "ul", 2) || sv_eq(tag, len, "ol", 2)) {
                return block_kind::list_close;
            }
            if (sv_eq(tag, len, "columns", 7)) {
                return block_kind::columns_close;
            }
            if (sv_starts_with(tag, len, "pad=", 4) || sv_eq(tag, len, "pad", 3)) {
                return block_kind::pad_close;
            }
            if (sv_eq(tag, len, "link", 4) || sv_starts_with(tag, len, "link=", 5)) {
                return block_kind::link_close;
            }
            return block_kind::none;
        }

        template <size_t N, size_t MaxFragments>
        consteval static_rich_markup<MaxFragments> ct_parse_rich_impl(const char (&markup)[N]) {
            static_rich_markup<MaxFragments> result{};
            constexpr size_t len = N - 1;

            style style_stack[32]{};
            size_t stack_depth = 1;

            size_t text_start = 0;
            bool in_text = false;
            size_t i = 0;

            auto flush_text = [&](size_t end) {
                if (in_text && end > text_start && result.count < MaxFragments) {
                    result.fragments[result.count].offset = text_start;
                    result.fragments[result.count].length = end - text_start;
                    result.fragments[result.count].sty = style_stack[stack_depth - 1];
                    // block.kind stays none (inline text)
                    result.count++;
                    in_text = false;
                }
            };

            while (i < len) {
                if (markup[i] == '[') {
                    if (i + 1 < len && markup[i + 1] == '[') {
                        if (!in_text) {
                            text_start = i;
                            in_text = true;
                        }
                        i += 2;
                        continue;
                    }

                    size_t close = i + 1;
                    while (close < len && markup[close] != ']') {
                        ++close;
                    }
                    if (close >= len) {
                        if (!in_text) {
                            text_start = i;
                            in_text = true;
                        }
                        ++i;
                        continue;
                    }

                    flush_text(i);

                    size_t tag_start = i + 1;
                    size_t tag_len = close - tag_start;

                    if (tag_len == 1 && markup[tag_start] == '/') {
                        // [/] — reset
                        stack_depth = 1;
                        style_stack[0] = style{};
                    } else if (tag_len > 0 && markup[tag_start] == '/') {
                        // [/tag] — closing tag
                        const char *close_name = markup + tag_start + 1;
                        size_t close_name_len = tag_len - 1;

                        block_kind ck = closing_block_kind(close_name, close_name_len);
                        if (ck != block_kind::none && result.count < MaxFragments) {
                            result.fragments[result.count].block.kind = ck;
                            result.count++;
                        }

                        if (stack_depth > 1) {
                            stack_depth--;
                        }
                    } else {
                        // [tag] — opening tag
                        // Try block tag first
                        ct_rich_fragment block_frag{};
                        if (try_block_tag_ct(markup + tag_start, tag_len, markup, tag_start, block_frag)) {
                            if (result.count < MaxFragments) {
                                result.fragments[result.count] = block_frag;
                                result.count++;
                            }
                            // For heading, also push style
                            if (block_frag.block.kind == block_kind::heading) {
                                if (stack_depth < 32) {
                                    style_stack[stack_depth] = block_frag.block.content_sty;
                                    stack_depth++;
                                }
                            }
                        } else {
                            // Inline style tag
                            if (stack_depth < 32) {
                                style_stack[stack_depth] = style_stack[stack_depth - 1];
                                apply_compound_tag_ct(markup + tag_start, tag_len, style_stack[stack_depth]);
                                stack_depth++;
                            }
                        }
                    }

                    i = close + 1;
                } else {
                    if (!in_text) {
                        text_start = i;
                        in_text = true;
                    }
                    ++i;
                }
            }

            flush_text(len);
            return result;
        }

    } // namespace detail

    /**
     * @brief Parse a rich markup string literal at compile time.
     *
     * Returns a static_rich_markup with both inline text fragments and
     * block-level descriptors. Each fragment's block.kind field indicates
     * whether it's inline text (none) or a block operation.
     */
    template <size_t MaxFragments = 64, size_t N>
    consteval static_rich_markup<MaxFragments> compile_markup(const char (&markup)[N]) {
        return detail::ct_parse_rich_impl<N, MaxFragments>(markup);
    }

} // namespace tapiru
