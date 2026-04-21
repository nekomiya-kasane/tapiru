/**
 * @file interactive.cpp
 * @brief Interactive widget builder flatten_into implementations.
 *
 * Each interactive builder flattens into text_builder nodes for rendering.
 * State is managed externally; builders just read current values.
 */

#include "tapiru/widgets/interactive.h"

#include "detail/scene.h"
#include "detail/widget_types.h"

#include <cstdio>
#include <string>

namespace tapiru {

    // ── button_builder ─────────────────────────────────────────────────────

    node_id button_builder::flatten_into(detail::scene &s) const {
        std::string markup;
        if (focused_) {
            markup = "[bold reverse] " + label_ + " [/]";
        } else {
            markup = "[bold] " + label_ + " [/]";
        }
        auto tb = text_builder(markup);
        if (style_.fg.kind != tapiru::color_kind::default_color ||
            style_.bg.kind != tapiru::color_kind::default_color) {
            tb.style_override(style_);
        }
        return tb.flatten_into(s);
    }

    // ── checkbox_builder ───────────────────────────────────────────────────

    node_id checkbox_builder::flatten_into(detail::scene &s) const {
        bool checked = value_ ? *value_ : false;
        std::string markup;
        if (focused_) {
            markup = std::string("[bold reverse]") + (checked ? "[✓]" : "[ ]") + " " + label_ + "[/]";
        } else {
            markup = std::string(checked ? "[✓]" : "[ ]") + " " + label_;
        }
        auto tb = text_builder(markup);
        if (style_.fg.kind != tapiru::color_kind::default_color ||
            style_.bg.kind != tapiru::color_kind::default_color) {
            tb.style_override(style_);
        }
        return tb.flatten_into(s);
    }

    // ── radio_group_builder ────────────────────────────────────────────────

    node_id radio_group_builder::flatten_into(detail::scene &s) const {
        int sel = selected_ ? *selected_ : -1;
        // Build a multi-line text with radio buttons
        std::string markup;
        for (int i = 0; i < static_cast<int>(options_.size()); ++i) {
            if (i > 0) {
                markup += "\n";
            }
            bool is_selected = (i == sel);
            bool is_focused = (i == focused_idx_);
            if (is_focused) {
                markup += "[bold reverse]";
            }
            markup += is_selected ? "(●) " : "( ) ";
            markup += options_[i];
            if (is_focused) {
                markup += "[/]";
            }
        }
        auto tb = text_builder(markup);
        if (style_.fg.kind != tapiru::color_kind::default_color ||
            style_.bg.kind != tapiru::color_kind::default_color) {
            tb.style_override(style_);
        }
        return tb.flatten_into(s);
    }

    // ── selectable_list_builder ────────────────────────────────────────────

    node_id selectable_list_builder::flatten_into(detail::scene &s) const {
        int cur = cursor_ ? *cursor_ : 0;
        uint32_t total = static_cast<uint32_t>(items_.size());
        uint32_t vis = (visible_ > 0 && visible_ < total) ? visible_ : total;

        // Calculate visible window
        uint32_t start = 0;
        if (static_cast<uint32_t>(cur) >= vis) {
            start = static_cast<uint32_t>(cur) - vis + 1;
        }
        if (start + vis > total) {
            start = total > vis ? total - vis : 0;
        }

        std::string markup;
        for (uint32_t i = start; i < start + vis && i < total; ++i) {
            if (i > start) {
                markup += "\n";
            }
            bool is_cursor = (static_cast<int>(i) == cur);
            if (is_cursor) {
                markup += "[bold reverse]> " + items_[i] + "[/]";
            } else {
                markup += "  " + items_[i];
            }
        }
        auto tb = text_builder(markup);
        return tb.flatten_into(s);
    }

    // ── text_input_builder ─────────────────────────────────────────────────

    node_id text_input_builder::flatten_into(detail::scene &s) const {
        std::string content = buffer_ ? *buffer_ : "";
        std::string display;

        if (content.empty() && !placeholder_.empty() && !focused_) {
            display = "[dim]" + placeholder_ + "[/]";
        } else {
            // Pad or truncate to width
            if (content.size() < width_) {
                display = content + std::string(width_ - content.size(), ' ');
            } else {
                display = content.substr(0, width_);
            }

            if (focused_) {
                // Show cursor position with reverse video
                if (cursor_pos_ < display.size()) {
                    std::string before = display.substr(0, cursor_pos_);
                    std::string cursor_ch(1, display[cursor_pos_]);
                    std::string after = display.substr(cursor_pos_ + 1);
                    display = before + "[reverse]" + cursor_ch + "[/]" + after;
                }
                display = "[underline]" + display + "[/]";
            }
        }

        auto tb = text_builder(display);
        if (style_.fg.kind != tapiru::color_kind::default_color ||
            style_.bg.kind != tapiru::color_kind::default_color) {
            tb.style_override(style_);
        }
        return tb.flatten_into(s);
    }

    // ── slider_builder ─────────────────────────────────────────────────────

    node_id slider_builder::flatten_into(detail::scene &s) const {
        float val = value_ ? *value_ : min_;
        float range = max_ - min_;
        float ratio = (range > 0.0f) ? (val - min_) / range : 0.0f;
        if (ratio < 0.0f) {
            ratio = 0.0f;
        }
        if (ratio > 1.0f) {
            ratio = 1.0f;
        }

        uint32_t bar_w = width_;
        if (show_pct_) {
            bar_w = width_ > 5 ? width_ - 5 : 1; // reserve space for " XXX%"
        }

        uint32_t filled = static_cast<uint32_t>(ratio * static_cast<float>(bar_w) + 0.5f);
        if (filled > bar_w) {
            filled = bar_w;
        }

        // Build bar string using markup
        std::string bar = "[";
        // We can't easily use char32_t in markup, so build as plain text
        // and use text_builder directly
        std::string bar_text;
        bar_text += '[';
        for (uint32_t i = 0; i < filled; ++i) {
            // UTF-8 encode fill char
            if (fill_ < 0x80) {
                bar_text += static_cast<char>(fill_);
            } else if (fill_ < 0x800) {
                bar_text += static_cast<char>(0xC0 | (fill_ >> 6));
                bar_text += static_cast<char>(0x80 | (fill_ & 0x3F));
            } else {
                bar_text += static_cast<char>(0xE0 | (fill_ >> 12));
                bar_text += static_cast<char>(0x80 | ((fill_ >> 6) & 0x3F));
                bar_text += static_cast<char>(0x80 | (fill_ & 0x3F));
            }
        }
        for (uint32_t i = filled; i < bar_w; ++i) {
            if (empty_ < 0x80) {
                bar_text += static_cast<char>(empty_);
            } else if (empty_ < 0x800) {
                bar_text += static_cast<char>(0xC0 | (empty_ >> 6));
                bar_text += static_cast<char>(0x80 | (empty_ & 0x3F));
            } else {
                bar_text += static_cast<char>(0xE0 | (empty_ >> 12));
                bar_text += static_cast<char>(0x80 | ((empty_ >> 6) & 0x3F));
                bar_text += static_cast<char>(0x80 | (empty_ & 0x3F));
            }
        }
        bar_text += ']';

        if (show_pct_) {
            int pct = static_cast<int>(ratio * 100.0f + 0.5f);
            char pct_buf[8];
            std::snprintf(pct_buf, sizeof(pct_buf), " %3d%%", pct);
            bar_text += pct_buf;
        }

        std::string markup;
        if (focused_) {
            markup = "[bold]" + bar_text + "[/]";
        } else {
            markup = bar_text;
        }

        auto tb = text_builder(markup);
        if (style_.fg.kind != tapiru::color_kind::default_color ||
            style_.bg.kind != tapiru::color_kind::default_color) {
            tb.style_override(style_);
        }
        return tb.flatten_into(s);
    }

} // namespace tapiru
