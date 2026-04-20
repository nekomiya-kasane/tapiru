/**
 * @file console.cpp
 * @brief Console implementation: markup → style → ANSI → sink.
 */

#include "tapiru/core/console.h"

#include "detail/border_join.h"
#include "detail/scene.h"
#include "detail/widget_registry.h"
#include "tapiru/core/unicode_width.h"
#include "tapiru/text/highlighter.h"
#include "tapiru/text/markup.h"

#include <cstdio>
#include <iostream>

namespace tapiru {

// ── Constructors ────────────────────────────────────────────────────────

console::console() : basic_console() {}

console::console(console_config cfg) : basic_console(std::move(cfg)) {}

// ── Public API ──────────────────────────────────────────────────────────

void console::print(std::string_view markup) {
    if (auto_highlight_ && highlighter_ && color_enabled()) {
        // Parse markup first, then auto-highlight plain text fragments
        auto fragments = parse_markup(markup);

        std::string buf;
        buf.reserve(markup.size() * 2);

        for (const auto &frag : fragments) {
            if (frag.sty.is_default()) {
                // Plain text — apply highlighter with link support
                auto hl_frags = highlight_text_linked(frag.text, *highlighter_);
                for (const auto &hf : hl_frags) {
                    if (!hf.link.empty()) buf += ansi::hyperlink_open(hf.link);
                    emitter_.transition(hf.sty, buf);
                    buf += hf.text;
                    if (!hf.link.empty()) {
                        emitter_.reset(buf);
                        buf += ansi::hyperlink_close;
                    }
                }
            } else {
                // Already styled by markup — emit as-is
                emitter_.transition(frag.sty, buf);
                buf += frag.text;
            }
        }
        emitter_.reset(buf);
        buf += '\n';
        config_.sink(buf);
    } else {
        emit_fragments(markup, true);
    }
}

void console::print_inline(std::string_view markup) {
    emit_fragments(markup, false);
}

void console::set_highlighter(const highlighter *hl) noexcept {
    highlighter_ = hl;
}

void console::set_auto_highlight(bool enabled) noexcept {
    auto_highlight_ = enabled;
}

void console::print_highlighted(std::string_view text) {
    emit_highlighted_fragments(text, true);
}

void console::print_highlighted_inline(std::string_view text) {
    emit_highlighted_fragments(text, false);
}

void console::print_rich(std::string_view markup) {
    // Runtime path: parse markup into rich fragments using the existing
    // inline parser, then emit. For block tags, we do a simple scan.
    // This is a simplified runtime fallback — the constexpr template path
    // is preferred for string literals.
    emit_fragments(markup, true);
}

// ── Internal ────────────────────────────────────────────────────────────

void console::emit_highlighted_fragments(std::string_view text, bool append_newline) {
    std::string buf;
    buf.reserve(text.size() * 2);

    if (color_enabled() && highlighter_) {
        auto fragments = highlight_text_linked(text, *highlighter_);
        for (const auto &frag : fragments) {
            if (!frag.link.empty()) buf += ansi::hyperlink_open(frag.link);
            emitter_.transition(frag.sty, buf);
            buf += frag.text;
            if (!frag.link.empty()) {
                emitter_.reset(buf);
                buf += ansi::hyperlink_close;
            }
        }
        emitter_.reset(buf);
    } else {
        buf += text;
    }

    if (append_newline) buf += '\n';

    config_.sink(buf);
}

void console::emit_fragments(std::string_view markup, bool append_newline) {
    auto fragments = parse_markup(markup);

    std::string buf;
    buf.reserve(markup.size() * 2);

    if (color_enabled()) {
        for (const auto &frag : fragments) {
            emitter_.transition(frag.sty, buf);
            buf += frag.text;
        }
        emitter_.reset(buf);
    } else {
        for (const auto &frag : fragments) {
            buf += frag.text;
        }
    }

    if (append_newline) {
        buf += '\n';
    }

    config_.sink(buf);
}

// ── Widget rendering pipeline ────────────────────────────────────────────

namespace {

void append_utf8(std::string &out, char32_t cp) {
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

} // anonymous namespace

void console::render_widget_impl(const std::function<uint32_t(void *)> &flatten_fn, void * /*builder_ptr*/,
                                 uint32_t width) {
    render_widget_impl(flatten_fn, nullptr, width, 0);
}

void console::render_widget_impl(const std::function<uint32_t(void *)> &flatten_fn, void * /*builder_ptr*/,
                                 uint32_t width, uint32_t height) {
    // Determine output width
    if (width == 0) {
        auto sz = terminal::get_size();
        width = sz.width > 0 ? sz.width : 80;
    }

    // 1. Create scene and flatten builder into it
    detail::scene sc;
    auto root_id = flatten_fn(&sc);

    // 2. Measure — use bounded height when provided (enables flex layout)
    auto bc = (height > 0) ? box_constraints::tight(width, height) : box_constraints::loose(width, unbounded);
    auto m = detail::dispatch_measure(sc, root_id, bc);

    // 3. Create canvas and render
    canvas cv(m.width, m.height);
    rect area{0, 0, m.width, m.height};
    detail::dispatch_render(sc, root_id, cv, area, sc.styles());

    // 3b. Post-process: merge adjacent border characters
    detail::apply_border_joins(cv);

    // 4. Convert canvas to ANSI output
    std::string buf;
    buf.reserve(m.width * m.height * 4);

    ansi_emitter local_emitter;

    for (uint32_t y = 0; y < m.height; ++y) {
        for (uint32_t x = 0; x < m.width; ++x) {
            const auto &c = cv.get(x, y);
            if (c.width == 0) continue; // skip continuation cells

            if (color_enabled()) {
                auto sty = sc.styles().lookup(c.sid);
                local_emitter.transition(sty, buf);
            }

            if (c.codepoint == 0 || c.codepoint == U' ') {
                buf += ' ';
            } else {
                append_utf8(buf, c.codepoint);
            }
        }
        if (color_enabled()) {
            local_emitter.reset(buf);
        }
        buf += '\n';
    }

    config_.sink(buf);
}

// ── Canvas capture ────────────────────────────────────────────────────────

widget_canvas render_to_canvas_impl(const std::function<uint32_t(void *)> &flatten_fn, uint32_t width) {
    if (width == 0) {
        auto sz = terminal::get_size();
        width = sz.width > 0 ? sz.width : 80;
    }

    detail::scene sc;
    auto root_id = flatten_fn(&sc);

    auto bc = box_constraints::loose(width, unbounded);
    auto m = detail::dispatch_measure(sc, root_id, bc);

    canvas cv(m.width, m.height);
    rect area{0, 0, m.width, m.height};
    detail::dispatch_render(sc, root_id, cv, area, sc.styles());
    detail::apply_border_joins(cv);

    widget_canvas wc;
    wc.cv = std::move(cv);
    wc.styles = std::move(sc.styles());
    wc.width = m.width;
    wc.height = m.height;
    return wc;
}

std::string widget_canvas::row_text(uint32_t y) const {
    std::string out;
    if (y >= height) return out;
    for (uint32_t x = 0; x < width; ++x) {
        const auto &c = cv.get(x, y);
        if (c.width == 0) continue; // skip continuation cells
        char32_t cp = c.codepoint;
        if (cp == 0 || cp == U' ') {
            out += ' ';
        } else {
            append_utf8(out, cp);
        }
    }
    return out;
}

std::vector<std::string> widget_canvas::all_rows() const {
    std::vector<std::string> rows;
    rows.reserve(height);
    for (uint32_t y = 0; y < height; ++y) {
        rows.push_back(row_text(y));
    }
    return rows;
}

void widget_canvas::dump(std::ostream &os) const {
    os << "=== widget_canvas " << width << "x" << height << " ===\n";
    for (uint32_t y = 0; y < height; ++y) {
        os << std::format("{:3d}|", y) << row_text(y) << "|\n";
    }
    os << "=== end ===\n";
}

void widget_canvas::dump_styled(std::ostream &os) const {
    os << "=== widget_canvas " << width << "x" << height << " (styled) ===\n";
    for (uint32_t y = 0; y < height; ++y) {
        os << std::format("{:3d}|", y);
        for (uint32_t x = 0; x < width; ++x) {
            const auto &c = cv.get(x, y);
            if (c.width == 0) continue;
            char32_t cp = c.codepoint;
            if (cp == 0 || cp == U' ') {
                os << ' ';
            } else {
                std::string tmp;
                append_utf8(tmp, cp);
                os << tmp;
            }
        }
        os << "|\n";

        // Print style info for cells with non-default styles
        bool has_style = false;
        for (uint32_t x = 0; x < width; ++x) {
            const auto &c = cv.get(x, y);
            if (c.sid != default_style_id && c.width > 0) {
                if (!has_style) {
                    os << "    ";
                    has_style = true;
                }
                auto sty = styles.lookup(c.sid);
                os << std::format("[{},{}]fg=({},{},{})bg=({},{},{}) ", x, y, sty.fg.r, sty.fg.g, sty.fg.b, sty.bg.r,
                                  sty.bg.g, sty.bg.b);
            }
        }
        if (has_style) os << "\n";
    }
    os << "=== end ===\n";
}

} // namespace tapiru

// Global default console instance (no namespace qualifier needed).
tapiru::console tcon;
