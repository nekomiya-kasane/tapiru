#pragma once

/**
 * @file classic_app.h
 * @brief Full-screen classic application shell with menu bar, content area,
 *        status bar, text selection, and ANSI dropdown rendering.
 *
 * Provides a ready-made application loop: raw terminal input, alternate screen,
 * synchronized output, and diff-optimized redraw.  The user supplies menu
 * definitions, a content builder callback, and a status builder callback.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "tapiru/core/input.h"
#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/menu_bar.h"
#include "tapiru/widgets/status_bar.h"

namespace tapiru {

// ── Theme ──────────────────────────────────────────────────────────────

/**
 * @brief Visual theme for classic_app.
 *
 * Provides colour presets for every visual element.  Use the static factory
 * methods dark() / light() for built-in presets, or populate manually.
 */
struct TAPIRU_API classic_app_theme {
    style menu_bar_bg;
    style menu_bar_active;
    style dropdown_border;
    style dropdown_item;
    style dropdown_highlight;
    style dropdown_shortcut;
    style status_bar;
    style selection_highlight;
    style cursor_line;

    /** @brief VS Code dark+ inspired theme. */
    static classic_app_theme dark();
    /** @brief Light theme preset. */
    static classic_app_theme light();
};

// ── Callback types ─────────────────────────────────────────────────────

/** @brief Called when a menu item is activated (clicked or Enter). */
using menu_action_handler = std::function<void(int menu_index, int item_global_index,
                                               const std::string& label)>;

/** @brief Called each frame to build the content area. */
using content_builder_fn = std::function<void(rows_builder& content,
                                              int scroll_y, int viewport_h)>;

/** @brief Called each frame to build the status bar. */
using status_builder_fn = std::function<void(status_bar_builder& sb)>;

/**
 * @brief Called each frame after widget render to write raw ANSI overlays.
 *
 * Use this for Sixel images, OSC 8 hyperlinks, or any raw escape sequences
 * that cannot go through the widget pipeline.  The string is written directly
 * to stdout via console::write.
 */
using overlay_fn = std::function<void(std::string& buf, uint32_t term_w, uint32_t term_h,
                                      int scroll_y, int viewport_h)>;

// ── classic_app ────────────────────────────────────────────────────────

/**
 * @brief Full-screen terminal application shell.
 *
 * Layout (top to bottom):
 *   row 0           — menu bar
 *   rows 1..N       — user content (scrollable)
 *   last row        — status bar
 *
 * Features:
 *   - Multi-level dropdown menus with ANSI rendering
 *   - Mouse text selection with highlight overlay
 *   - Keyboard scroll (arrows, page up/down, home/end)
 *   - Synchronized output (DEC 2026) for flicker-free redraw
 *   - Alternate screen + hidden cursor
 */
class TAPIRU_API classic_app {
public:
    struct config {
        std::vector<menu_bar_entry> menus;
        classic_app_theme           theme = classic_app_theme::dark();
        int                         poll_interval_ms = 50;
    };

    explicit classic_app(config cfg);
    ~classic_app();

    classic_app(const classic_app&) = delete;
    classic_app& operator=(const classic_app&) = delete;
    classic_app(classic_app&&) noexcept;
    classic_app& operator=(classic_app&&) noexcept;

    // ── Callbacks ──────────────────────────────────────────────────────

    /** @brief Set the content builder (called every frame). */
    void set_content(content_builder_fn fn);

    /** @brief Set the status bar builder (called every frame). */
    void set_status(status_builder_fn fn);

    /** @brief Set a raw ANSI overlay callback (Sixel, OSC 8, etc.). */
    void set_overlay(overlay_fn fn);

    /** @brief Register a handler for menu item activation. */
    void on_menu_action(menu_action_handler fn);

    /**
     * @brief Register a handler for key events not consumed by the shell.
     * Return true from the callback to indicate the event was handled.
     */
    void on_key(std::function<bool(const key_event&)> fn);

    // ── Lifecycle ──────────────────────────────────────────────────────

    /** @brief Enter the main loop. Blocks until quit() is called. */
    void run();

    /** @brief Request the main loop to exit. Safe to call from callbacks. */
    void quit();

    // ── State queries (for use inside callbacks) ───────────────────────

    [[nodiscard]] int      scroll_y()    const;
    [[nodiscard]] int      viewport_h()  const;
    [[nodiscard]] int      cursor_line() const;
    [[nodiscard]] int      cursor_col()  const;
    [[nodiscard]] bool     has_selection() const;
    void                   selection_range(int& sl, int& sc, int& el, int& ec) const;
    [[nodiscard]] uint32_t term_width()  const;
    [[nodiscard]] uint32_t term_height() const;

    // ── State mutation (for use inside callbacks) ──────────────────────

    void scroll_to(int line);
    void set_status_message(const std::string& msg);
    void set_content_lines(int total);
    void set_document_lines(const std::vector<std::string>& lines);

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}  // namespace tapiru
