#pragma once

/**
 * @file render_op.h
 * @brief Constexpr render operation infrastructure for extended markup.
 *
 * Each markup fragment compiles down to a render op — a function pointer
 * whose compile-time config is baked into its template instantiation.
 * At runtime, each op is called with a single extensible markup_render_context&.
 *
 * Usage:
 *   constexpr auto plan = tapiru::compile_markup("[box]Hello[/box]");
 *   tapiru::markup_render_context ctx{...};
 *   tapiru::execute(plan, ctx);
 */

#include "tapiru/core/ansi.h"
#include "tapiru/core/style.h"
#include "tapiru/layout/types.h"
#include "tapiru/text/constexpr_markup.h"
#include "tapiru/text/emoji.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace tapiru {

// ── Runtime context ─────────────────────────────────────────────────────

/**
 * @brief Runtime context passed to every render op.
 *
 * Extensible: add fields freely without breaking existing ops.
 */
struct markup_render_context {
    const char *src = nullptr;       ///< Original markup string literal
    uint32_t width = 80;             ///< Terminal width
    bool color_on = true;            ///< Whether to emit ANSI color codes
    ansi_emitter *emitter = nullptr; ///< ANSI state tracker
    std::string *out = nullptr;      ///< Output buffer

    // Block state (managed by open/close ops)
    std::string line_prefix;    ///< Accumulated indent/quote/box prefix
    std::string line_suffix;    ///< Box suffix
    uint32_t content_width = 0; ///< Available width after prefix/suffix
    uint8_t list_counter = 0;   ///< Current list item number
    bool list_ordered = false;  ///< Whether current list is ordered
};

// ── Render op function pointer type ─────────────────────────────────────

using render_op_fn = void (*)(markup_render_context &);

// ── Render plan ─────────────────────────────────────────────────────────

template <size_t MaxOps = 64> struct static_render_plan {
    std::array<render_op_fn, MaxOps> ops{};
    size_t count = 0;
};

// ── Execute ─────────────────────────────────────────────────────────────

template <size_t MaxOps> inline void execute(const static_render_plan<MaxOps> &plan, markup_render_context &ctx) {
    for (size_t i = 0; i < plan.count; ++i) {
        plan.ops[i](ctx);
    }
}

// ── UTF-8 encoding helper ───────────────────────────────────────────────

namespace detail {

inline void append_utf8(std::string &out, char32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

inline void append_border_char(std::string &out, char32_t ch) {
    append_utf8(out, ch);
}

inline void repeat_char(std::string &out, char32_t ch, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        append_utf8(out, ch);
    }
}

} // namespace detail

// ═══════════════════════════════════════════════════════════════════════
// Pure compile-time ops (output is fixed bytes, no ctx.width needed)
// ═══════════════════════════════════════════════════════════════════════

// ── op_text: emit styled text from source literal ───────────────────────

template <size_t Offset, size_t Length, style Sty> inline void op_text(markup_render_context &ctx) {
    if (ctx.color_on && ctx.emitter) {
        ctx.emitter->transition(Sty, *ctx.out);
    }
    ctx.out->append(ctx.src + Offset, Length);
}

// ── op_break: emit newline ──────────────────────────────────────────────

inline void op_break(markup_render_context &ctx) {
    *ctx.out += '\n';
}

// ── op_spacing: emit N spaces ───────────────────────────────────────────

template <uint8_t N> inline void op_spacing(markup_render_context &ctx) {
    ctx.out->append(N, ' ');
}

// ── op_emoji: emit pre-resolved emoji codepoint as UTF-8 ────────────────

template <char32_t Codepoint> inline void op_emoji(markup_render_context &ctx) {
    detail::append_utf8(*ctx.out, Codepoint);
}

// ── op_style_push/pop: transition to/from a style ───────────────────────

template <style Sty> inline void op_style_push(markup_render_context &ctx) {
    if (ctx.color_on && ctx.emitter) {
        ctx.emitter->transition(Sty, *ctx.out);
    }
}

template <style Sty> inline void op_style_pop(markup_render_context &ctx) {
    if (ctx.color_on && ctx.emitter) {
        ctx.emitter->transition(Sty, *ctx.out);
    }
}

// ── op_link_open/close: OSC 8 hyperlink ─────────────────────────────────

template <size_t UrlOffset, size_t UrlLength> inline void op_link_open(markup_render_context &ctx) {
    if (ctx.color_on) {
        *ctx.out += "\033]8;;";
        ctx.out->append(ctx.src + UrlOffset, UrlLength);
        *ctx.out += "\033\\";
    }
}

inline void op_link_close(markup_render_context &ctx) {
    if (ctx.color_on) {
        *ctx.out += "\033]8;;\033\\";
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Width-dependent ops (read ctx.width for layout)
// ═══════════════════════════════════════════════════════════════════════

// ── op_box_open/close ───────────────────────────────────────────────────

template <border_style BS, size_t TitleOffset = 0, size_t TitleLength = 0>
inline void op_box_open(markup_render_context &ctx) {
    constexpr auto chars = get_border_chars(BS);
    uint32_t w = ctx.content_width > 0 ? ctx.content_width : ctx.width;
    if (w < 4) {
        w = 4;
    }

    // Top border: ╭─── Title ───╮
    detail::append_border_char(*ctx.out, chars.top_left);
    if constexpr (TitleLength > 0) {
        detail::append_border_char(*ctx.out, chars.horizontal);
        *ctx.out += ' ';
        ctx.out->append(ctx.src + TitleOffset, TitleLength);
        *ctx.out += ' ';
        uint32_t used = 4 + static_cast<uint32_t>(TitleLength);
        if (w > used) {
            detail::repeat_char(*ctx.out, chars.horizontal, w - used);
        }
    } else {
        if (w > 2) {
            detail::repeat_char(*ctx.out, chars.horizontal, w - 2);
        }
    }
    detail::append_border_char(*ctx.out, chars.top_right);
    *ctx.out += '\n';

    // Set up line prefix/suffix for content
    ctx.line_prefix.clear();
    detail::append_utf8(ctx.line_prefix, chars.vertical);
    ctx.line_prefix += ' ';
    ctx.line_suffix.clear();
    ctx.line_suffix += ' ';
    detail::append_utf8(ctx.line_suffix, chars.vertical);
    ctx.content_width = (w > 4) ? (w - 4) : 1;

    // Emit line prefix for first content line
    *ctx.out += ctx.line_prefix;
}

template <border_style BS> inline void op_box_close(markup_render_context &ctx) {
    constexpr auto chars = get_border_chars(BS);
    uint32_t w = ctx.content_width + 4;

    // Close current content line
    // Pad remaining content width
    *ctx.out += ctx.line_suffix;
    *ctx.out += '\n';

    // Bottom border: ╰────────────╯
    detail::append_border_char(*ctx.out, chars.bottom_left);
    if (w > 2) {
        detail::repeat_char(*ctx.out, chars.horizontal, w - 2);
    }
    detail::append_border_char(*ctx.out, chars.bottom_right);

    // Reset state
    ctx.line_prefix.clear();
    ctx.line_suffix.clear();
    ctx.content_width = 0;
}

// ── op_rule: horizontal rule ────────────────────────────────────────────

template <size_t TitleOffset = 0, size_t TitleLength = 0> inline void op_rule(markup_render_context &ctx) {
    constexpr char32_t rule_char = U'\x2500'; // ─
    uint32_t w = ctx.content_width > 0 ? ctx.content_width : ctx.width;

    if constexpr (TitleLength > 0) {
        // ── Title ──
        uint32_t title_len = static_cast<uint32_t>(TitleLength);
        uint32_t decor = title_len + 4; // "── " + title + " ──"
        uint32_t left = 2;
        detail::repeat_char(*ctx.out, rule_char, left);
        *ctx.out += ' ';
        ctx.out->append(ctx.src + TitleOffset, TitleLength);
        *ctx.out += ' ';
        if (w > decor) {
            detail::repeat_char(*ctx.out, rule_char, w - decor);
        }
    } else {
        detail::repeat_char(*ctx.out, rule_char, w);
    }
}

// ── op_indent_open/close ────────────────────────────────────────────────

template <uint8_t N = 4> inline void op_indent_open(markup_render_context &ctx) {
    ctx.line_prefix.append(N, ' ');
    if (ctx.content_width > N) {
        ctx.content_width -= N;
    }
    *ctx.out += std::string(N, ' ');
}

inline void op_indent_close(markup_render_context &ctx) {
    // Simple: just clear prefix (doesn't handle nested indents perfectly,
    // but sufficient for streaming markup)
    ctx.line_prefix.clear();
    ctx.content_width = 0;
}

// ── op_align_open/close ─────────────────────────────────────────────────

template <justify J> inline void op_align_open(markup_render_context & /*ctx*/) {
    // Alignment is handled by buffering — for streaming output,
    // we just note the mode. Full implementation would buffer content.
}

template <justify J> inline void op_align_close(markup_render_context & /*ctx*/) {}

// ── op_quote_open/close ─────────────────────────────────────────────────

inline void op_quote_open(markup_render_context &ctx) {
    // Prepend │ with dim cyan style
    if (ctx.color_on && ctx.emitter) {
        style quote_sty{colors::bright_black, {}, attr::none};
        ctx.emitter->transition(quote_sty, *ctx.out);
    }
    detail::append_utf8(*ctx.out, U'\x2502'); // │
    *ctx.out += ' ';
    if (ctx.color_on && ctx.emitter) {
        ctx.emitter->reset(*ctx.out);
    }
    ctx.line_prefix.clear();
    detail::append_utf8(ctx.line_prefix, U'\x2502');
    ctx.line_prefix += ' ';
}

inline void op_quote_close(markup_render_context &ctx) {
    ctx.line_prefix.clear();
}

// ── op_code_open/close ──────────────────────────────────────────────────

inline void op_code_open(markup_render_context &ctx) {
    if (ctx.color_on && ctx.emitter) {
        style code_sty{{}, color::from_rgb(0x1e, 0x1e, 0x2e), attr::none};
        ctx.emitter->transition(code_sty, *ctx.out);
    }
    ctx.line_prefix.append(2, ' ');
    *ctx.out += "  ";
}

inline void op_code_close(markup_render_context &ctx) {
    if (ctx.color_on && ctx.emitter) {
        ctx.emitter->reset(*ctx.out);
    }
    ctx.line_prefix.clear();
}

// ── op_list_open/close ──────────────────────────────────────────────────

template <bool Ordered> inline void op_list_open(markup_render_context &ctx) {
    ctx.list_ordered = Ordered;
    ctx.list_counter = 0;
}

inline void op_list_close(markup_render_context &ctx) {
    ctx.list_counter = 0;
    ctx.list_ordered = false;
}

// ── op_list_item: prepend bullet/number ─────────────────────────────────

template <bool Ordered> inline void op_list_item(markup_render_context &ctx) {
    if constexpr (Ordered) {
        ++ctx.list_counter;
        *ctx.out += std::to_string(ctx.list_counter);
        *ctx.out += ". ";
    } else {
        detail::append_utf8(*ctx.out, U'\x2022'); // •
        *ctx.out += ' ';
    }
}

// ── op_pad_open/close ───────────────────────────────────────────────────

template <uint8_t N> inline void op_pad_open(markup_render_context &ctx) {
    ctx.out->append(N, ' ');
}

template <uint8_t N> inline void op_pad_close(markup_render_context &ctx) {
    ctx.out->append(N, ' ');
}

// ── op_progress: inline progress bar ────────────────────────────────────

template <uint8_t Percent> inline void op_progress(markup_render_context &ctx) {
    constexpr uint8_t pct = Percent > 100 ? 100 : Percent;
    uint32_t bar_width = ctx.content_width > 0 ? ctx.content_width : ctx.width;
    if (bar_width > 6) {
        bar_width -= 6; // room for " XXX%"
    }
    uint32_t filled = bar_width * pct / 100;
    uint32_t empty = bar_width - filled;

    detail::repeat_char(*ctx.out, U'\x2588', filled); // █
    detail::repeat_char(*ctx.out, U'\x2591', empty);  // ░
    *ctx.out += ' ';
    if constexpr (pct < 10) {
        *ctx.out += "  ";
    } else if constexpr (pct < 100) {
        *ctx.out += ' ';
    }
    *ctx.out += std::to_string(pct);
    *ctx.out += '%';
}

// ── op_bar: fraction bar ────────────────────────────────────────────────

template <uint8_t Num, uint8_t Den> inline void op_bar(markup_render_context &ctx) {
    static_assert(Den > 0, "Denominator must be > 0");
    constexpr uint8_t num = Num > Den ? Den : Num;
    uint32_t bar_width = ctx.content_width > 0 ? ctx.content_width : ctx.width;
    if (bar_width > 6) {
        bar_width -= 6;
    }
    uint32_t filled = bar_width * num / Den;
    uint32_t empty = bar_width - filled;

    detail::repeat_char(*ctx.out, U'\x2588', filled);
    detail::repeat_char(*ctx.out, U'\x2591', empty);
    *ctx.out += ' ';
    *ctx.out += std::to_string(num);
    *ctx.out += '/';
    *ctx.out += std::to_string(Den);
}

// ── op_columns_open/close (simplified: split by | in content) ───────────

inline void op_columns_open(markup_render_context & /*ctx*/) {
    // In streaming mode, columns are best-effort.
    // Full implementation would buffer content and split on |.
}

inline void op_columns_close(markup_render_context & /*ctx*/) {}

// ═══════════════════════════════════════════════════════════════════════
// Descriptor dispatch: interprets ct_rich_fragment at runtime
// ═══════════════════════════════════════════════════════════════════════

/**
 * @brief Dispatch a single ct_rich_fragment descriptor to the appropriate
 *        render operation at runtime.
 *
 * This bridges the constexpr descriptor world (compile_markup output) to
 * the runtime markup_render_context world. The switch is in one place; each case
 * delegates to the cleanly separated op functions above.
 */
inline void dispatch_fragment(const ct_rich_fragment &frag, markup_render_context &ctx) {
    switch (frag.block.kind) {
    case block_kind::none:
        // Inline text
        if (ctx.color_on && ctx.emitter) {
            ctx.emitter->transition(frag.sty, *ctx.out);
        }
        ctx.out->append(ctx.src + frag.offset, frag.length);
        break;

    case block_kind::line_break:
        *ctx.out += '\n';
        if (!ctx.line_prefix.empty()) {
            *ctx.out += ctx.line_prefix;
        }
        break;

    case block_kind::spacing:
        ctx.out->append(frag.block.level, ' ');
        break;

    case block_kind::emoji: {
        // Resolve emoji at runtime via emoji_lookup
        std::string shortcode = ":";
        shortcode.append(ctx.src + frag.block.name_offset, frag.block.name_length);
        shortcode += ':';
        char32_t cp = emoji_lookup(shortcode);
        if (cp != 0) {
            detail::append_utf8(*ctx.out, cp);
        }
        break;
    }

    case block_kind::heading:
        if (ctx.color_on && ctx.emitter) {
            ctx.emitter->transition(frag.block.content_sty, *ctx.out);
        }
        break;

    case block_kind::heading_close:
        if (ctx.color_on && ctx.emitter) {
            ctx.emitter->reset(*ctx.out);
        }
        break;

    case block_kind::link_open:
        if (ctx.color_on) {
            *ctx.out += "\033]8;;";
            ctx.out->append(ctx.src + frag.block.url_offset, frag.block.url_length);
            *ctx.out += "\033\\";
        }
        break;

    case block_kind::link_close:
        if (ctx.color_on) {
            *ctx.out += "\033]8;;\033\\";
        }
        break;

    case block_kind::box_open: {
        auto bs = static_cast<border_style>(frag.block.border);
        auto chars = get_border_chars(bs);
        uint32_t w = ctx.content_width > 0 ? ctx.content_width : ctx.width;
        if (w < 4) {
            w = 4;
        }

        detail::append_border_char(*ctx.out, chars.top_left);
        if (frag.block.title_length > 0) {
            detail::append_border_char(*ctx.out, chars.horizontal);
            *ctx.out += ' ';
            ctx.out->append(ctx.src + frag.block.title_offset, frag.block.title_length);
            *ctx.out += ' ';
            uint32_t used = 4 + frag.block.title_length;
            if (w > used) {
                detail::repeat_char(*ctx.out, chars.horizontal, w - used);
            }
        } else {
            if (w > 2) {
                detail::repeat_char(*ctx.out, chars.horizontal, w - 2);
            }
        }
        detail::append_border_char(*ctx.out, chars.top_right);
        *ctx.out += '\n';

        ctx.line_prefix.clear();
        detail::append_utf8(ctx.line_prefix, chars.vertical);
        ctx.line_prefix += ' ';
        ctx.line_suffix.clear();
        ctx.line_suffix += ' ';
        detail::append_utf8(ctx.line_suffix, chars.vertical);
        ctx.content_width = (w > 4) ? (w - 4) : 1;
        *ctx.out += ctx.line_prefix;
        break;
    }

    case block_kind::box_close: {
        auto bs = static_cast<border_style>(frag.block.border);
        // Try to recover border from line_prefix (it was set by box_open)
        // Use the same border_style that was used for open
        // For simplicity, re-derive from the stored border value
        // But frag.block.border is 0 for close tags — we need to get it from context
        // For now, use rounded as fallback
        border_style actual_bs = border_style::rounded;
        // Check if line_suffix is non-empty (we're inside a box)
        if (!ctx.line_suffix.empty()) {
            // We stored the border chars in prefix/suffix, so we can just close
            actual_bs = bs; // bs will be 0 (default) for close tags
        }
        auto chars = get_border_chars(actual_bs);
        uint32_t w = ctx.content_width + 4;

        *ctx.out += ctx.line_suffix;
        *ctx.out += '\n';
        detail::append_border_char(*ctx.out, chars.bottom_left);
        if (w > 2) {
            detail::repeat_char(*ctx.out, chars.horizontal, w - 2);
        }
        detail::append_border_char(*ctx.out, chars.bottom_right);

        ctx.line_prefix.clear();
        ctx.line_suffix.clear();
        ctx.content_width = 0;
        break;
    }

    case block_kind::rule: {
        constexpr char32_t rule_char = U'\x2500';
        uint32_t w = ctx.content_width > 0 ? ctx.content_width : ctx.width;
        if (frag.block.title_length > 0) {
            detail::repeat_char(*ctx.out, rule_char, 2);
            *ctx.out += ' ';
            ctx.out->append(ctx.src + frag.block.title_offset, frag.block.title_length);
            *ctx.out += ' ';
            uint32_t decor = frag.block.title_length + 4;
            if (w > decor) {
                detail::repeat_char(*ctx.out, rule_char, w - decor);
            }
        } else {
            detail::repeat_char(*ctx.out, rule_char, w);
        }
        break;
    }

    case block_kind::indent_open:
        ctx.out->append(frag.block.level, ' ');
        ctx.line_prefix.append(frag.block.level, ' ');
        if (ctx.content_width > frag.block.level) {
            ctx.content_width -= frag.block.level;
        }
        break;

    case block_kind::indent_close:
        ctx.line_prefix.clear();
        ctx.content_width = 0;
        break;

    case block_kind::align_open:
    case block_kind::align_close:
        // Alignment in streaming mode is best-effort
        break;

    case block_kind::quote_open:
        if (ctx.color_on && ctx.emitter) {
            style quote_sty{colors::bright_black, {}, attr::none};
            ctx.emitter->transition(quote_sty, *ctx.out);
        }
        detail::append_utf8(*ctx.out, U'\x2502');
        *ctx.out += ' ';
        if (ctx.color_on && ctx.emitter) {
            ctx.emitter->reset(*ctx.out);
        }
        ctx.line_prefix.clear();
        detail::append_utf8(ctx.line_prefix, U'\x2502');
        ctx.line_prefix += ' ';
        break;

    case block_kind::quote_close:
        ctx.line_prefix.clear();
        break;

    case block_kind::code_open:
        if (ctx.color_on && ctx.emitter) {
            style code_sty{{}, color::from_rgb(0x1e, 0x1e, 0x2e), attr::none};
            ctx.emitter->transition(code_sty, *ctx.out);
        }
        ctx.line_prefix.append(2, ' ');
        *ctx.out += "  ";
        break;

    case block_kind::code_close:
        if (ctx.color_on && ctx.emitter) {
            ctx.emitter->reset(*ctx.out);
        }
        ctx.line_prefix.clear();
        break;

    case block_kind::list_open:
        ctx.list_ordered = frag.block.ordered;
        ctx.list_counter = 0;
        break;

    case block_kind::list_close:
        ctx.list_counter = 0;
        ctx.list_ordered = false;
        break;

    case block_kind::list_item:
        if (ctx.list_ordered) {
            ++ctx.list_counter;
            *ctx.out += std::to_string(ctx.list_counter);
            *ctx.out += ". ";
        } else {
            detail::append_utf8(*ctx.out, U'\x2022');
            *ctx.out += ' ';
        }
        break;

    case block_kind::pad_open:
        ctx.out->append(frag.block.level, ' ');
        break;

    case block_kind::pad_close:
        ctx.out->append(frag.block.level, ' ');
        break;

    case block_kind::progress_bar: {
        uint8_t pct = frag.block.numerator > 100 ? 100 : frag.block.numerator;
        uint32_t bar_w = ctx.content_width > 0 ? ctx.content_width : ctx.width;
        if (bar_w > 6) {
            bar_w -= 6;
        }
        uint32_t filled = bar_w * pct / 100;
        uint32_t empty_part = bar_w - filled;
        detail::repeat_char(*ctx.out, U'\x2588', filled);
        detail::repeat_char(*ctx.out, U'\x2591', empty_part);
        *ctx.out += ' ';
        *ctx.out += std::to_string(pct);
        *ctx.out += '%';
        break;
    }

    case block_kind::fraction_bar: {
        uint8_t den = frag.block.denominator > 0 ? frag.block.denominator : 1;
        uint8_t num = frag.block.numerator > den ? den : frag.block.numerator;
        uint32_t bar_w = ctx.content_width > 0 ? ctx.content_width : ctx.width;
        if (bar_w > 6) {
            bar_w -= 6;
        }
        uint32_t filled = bar_w * num / den;
        uint32_t empty_part = bar_w - filled;
        detail::repeat_char(*ctx.out, U'\x2588', filled);
        detail::repeat_char(*ctx.out, U'\x2591', empty_part);
        *ctx.out += ' ';
        *ctx.out += std::to_string(num);
        *ctx.out += '/';
        *ctx.out += std::to_string(den);
        break;
    }

    case block_kind::columns_open:
    case block_kind::columns_close:
        break;
    }
}

/**
 * @brief Execute a compile-time rich markup plan at runtime.
 *
 * Iterates over all fragments in the plan and dispatches each one.
 */
template <size_t MaxFragments>
inline void execute_rich(const static_rich_markup<MaxFragments> &plan, markup_render_context &ctx) {
    for (size_t i = 0; i < plan.count; ++i) {
        dispatch_fragment(plan.fragments[i], ctx);
    }
}

} // namespace tapiru
