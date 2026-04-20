#pragma once

/**
 * @file test_harness.h
 * @brief Non-interactive TUI test harness for tapiru widgets and classic_app.
 *
 * Provides:
 *   - virtual_screen: render any widget builder to a cell grid, extract plain
 *     text rows, query individual cells by (x,y), dump the full screen.
 *   - app_harness: drive a classic_app without a real terminal — inject fake
 *     key/mouse events, capture each rendered frame, assert on screen content.
 *
 * Usage (widget-level):
 *   tapiru::testing::virtual_screen vs(80, 24);
 *   vs.render(my_rows_builder);
 *   auto row0 = vs.row_text(0);          // plain text of row 0
 *   auto c    = vs.cell_at(5, 2);        // cell at column 5, row 2
 *   vs.dump(std::cerr);                  // print full screen to stream
 *
 * Usage (app-level):
 *   tapiru::testing::app_harness ah(80, 24, app_config);
 *   ah.set_content(...);
 *   ah.set_status(...);
 *   ah.render();                          // render one frame
 *   ah.inject(key_event{'q'});            // inject a key press
 *   ah.render();                          // render after event
 *   ah.screen().dump(std::cerr);
 */

#include "tapiru/core/canvas.h"
#include "tapiru/core/console.h"
#include "tapiru/core/input.h"
#include "tapiru/core/style.h"
#include "tapiru/layout/types.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/classic_app.h"
#include "tapiru/widgets/menu_bar.h"
#include "tapiru/widgets/status_bar.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace tapiru::testing {

// ── virtual_screen ──────────────────────────────────────────────────────

/**
 * @brief Captures a rendered widget tree into a queryable cell grid.
 *
 * Renders via console::print_widget() with color disabled, capturing the
 * plain-text output. Also provides a structured cell grid for per-cell
 * inspection when color is enabled.
 */
class virtual_screen {
  public:
    explicit virtual_screen(uint32_t width = 80, uint32_t height = 24) : width_(width), height_(height) {}

    /** @brief Render a widget builder into this screen. */
    template <typename Builder> void render(const Builder &builder) {
        // Capture raw ANSI output
        raw_.clear();
        console_config cfg;
        cfg.sink = [this](std::string_view data) { raw_ += data; };
        cfg.depth = color_depth::none;
        cfg.no_color = true;
        console con(cfg);
        con.print_widget(builder, width_);

        // Parse raw output into rows
        rows_.clear();
        std::string current_row;
        for (char ch : raw_) {
            if (ch == '\n') {
                rows_.push_back(std::move(current_row));
                current_row.clear();
            } else {
                current_row += ch;
            }
        }
        if (!current_row.empty()) {
            rows_.push_back(std::move(current_row));
        }
    }

    /** @brief Number of rendered rows. */
    [[nodiscard]] uint32_t row_count() const noexcept { return static_cast<uint32_t>(rows_.size()); }

    /** @brief Plain text content of a specific row. */
    [[nodiscard]] const std::string &row_text(uint32_t y) const {
        assert(y < rows_.size());
        return rows_[y];
    }

    /** @brief Check if any row contains the given substring. */
    [[nodiscard]] bool contains(std::string_view text) const {
        for (auto &row : rows_) {
            if (row.find(text) != std::string::npos) return true;
        }
        return false;
    }

    /** @brief Find the first row containing the given substring. Returns -1 if not found. */
    [[nodiscard]] int find_row(std::string_view text) const {
        for (size_t i = 0; i < rows_.size(); ++i) {
            if (rows_[i].find(text) != std::string::npos) return static_cast<int>(i);
        }
        return -1;
    }

    /** @brief Find the column position of text in a given row. Returns -1 if not found. */
    [[nodiscard]] int find_col(uint32_t row, std::string_view text) const {
        if (row >= rows_.size()) return -1;
        auto pos = rows_[row].find(text);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }

    /** @brief Get the configured width. */
    [[nodiscard]] uint32_t width() const noexcept { return width_; }

    /** @brief Get the configured height. */
    [[nodiscard]] uint32_t height() const noexcept { return height_; }

    /** @brief Get the raw captured output. */
    [[nodiscard]] const std::string &raw() const noexcept { return raw_; }

    /** @brief Dump the screen to an output stream with row numbers. */
    void dump(std::ostream &os) const {
        os << "=== virtual_screen " << width_ << "x" << rows_.size() << " ===\n";
        for (size_t i = 0; i < rows_.size(); ++i) {
            os << std::format("{:3d}|", i) << rows_[i] << "|\n";
        }
        os << "=== end ===\n";
    }

    /** @brief Render a widget builder to canvas for cell-level inspection. */
    template <typename Builder> void render_canvas(const Builder &builder) {
        wc_ = render_to_canvas(builder, width_);
        // Also update text rows from canvas
        rows_.clear();
        if (wc_.height > 0) {
            rows_ = wc_.all_rows();
        }
    }

    /** @brief Access the widget_canvas (only valid after render_canvas). */
    [[nodiscard]] const widget_canvas &canvas() const noexcept { return wc_; }

    /** @brief Check if canvas is available. */
    [[nodiscard]] bool has_canvas() const noexcept { return wc_.width > 0; }

    /** @brief Dump the screen with a ruler showing column positions. */
    void dump_with_ruler(std::ostream &os) const {
        // Tens ruler
        os << "   |";
        for (uint32_t x = 0; x < width_; ++x) {
            os << ((x % 10 == 0) ? static_cast<char>('0' + (x / 10) % 10) : ' ');
        }
        os << "|\n";
        // Ones ruler
        os << "   |";
        for (uint32_t x = 0; x < width_; ++x) {
            os << static_cast<char>('0' + x % 10);
        }
        os << "|\n";
        // Separator
        os << "---+" << std::string(width_, '-') << "+\n";
        // Rows
        for (size_t i = 0; i < rows_.size(); ++i) {
            os << std::format("{:3d}|", i) << rows_[i];
            // Pad to width
            if (rows_[i].size() < width_) {
                os << std::string(width_ - rows_[i].size(), ' ');
            }
            os << "|\n";
        }
        os << "---+" << std::string(width_, '-') << "+\n";
    }

  private:
    uint32_t width_;
    uint32_t height_;
    std::string raw_;
    std::vector<std::string> rows_;
    widget_canvas wc_;
};

// ── app_harness ─────────────────────────────────────────────────────────

/**
 * @brief Drives a classic_app-like frame without a real terminal.
 *
 * Builds the same widget tree as classic_app::base_view (menu bar + content
 * + status bar) and renders it to a virtual_screen. Supports injecting
 * fake input events for testing interactive behavior.
 */
class app_harness {
  public:
    using content_fn = std::function<void(rows_builder &content, int scroll_y, int viewport_h)>;
    using status_fn = std::function<void(status_bar_builder &sb)>;

    struct config {
        uint32_t width = 80;
        uint32_t height = 24;
        std::vector<menu_bar_entry> menus;
        classic_app_theme theme = classic_app_theme::dark();
    };

    explicit app_harness(config cfg) : cfg_(std::move(cfg)), screen_(cfg_.width, cfg_.height) {}

    void set_content(content_fn fn) { content_fn_ = std::move(fn); }
    void set_status(status_fn fn) { status_fn_ = std::move(fn); }

    /** @brief Render one frame and capture to the virtual screen. */
    void render() {
        int viewport_h = static_cast<int>(cfg_.height) - 3; // match classic_app: menu + status + 1

        // Menu bar (same logic as classic_app::impl::build_menu_bar_row)
        std::string bar = " ";
        for (auto &m : cfg_.menus) {
            bar += "[rgb(" + std::to_string(cfg_.theme.menu_bar_bg.fg.r) + "," +
                   std::to_string(cfg_.theme.menu_bar_bg.fg.g) + "," + std::to_string(cfg_.theme.menu_bar_bg.fg.b) +
                   ") on_rgb(" + std::to_string(cfg_.theme.menu_bar_bg.bg.r) + "," +
                   std::to_string(cfg_.theme.menu_bar_bg.bg.g) + "," + std::to_string(cfg_.theme.menu_bar_bg.bg.b) +
                   ")] " + m.label + " [/]";
        }
        auto menu_bar = text_builder(bar);
        menu_bar.style_override(cfg_.theme.menu_bar_bg);

        // Content
        rows_builder content;
        content.gap(0);
        if (content_fn_) {
            content_fn_(content, scroll_y_, viewport_h);
        }

        // Status bar
        status_bar_builder sb;
        if (status_fn_) {
            status_fn_(sb);
        }
        sb.style_override(cfg_.theme.status_bar);

        // Assemble frame
        rows_builder frame;
        frame.gap(0);
        frame.add(std::move(menu_bar));
        frame.add(std::move(content));
        frame.add(std::move(sb));

        screen_.render(frame);
        frame_count_++;
    }

    /** @brief Access the captured screen. */
    [[nodiscard]] const virtual_screen &screen() const noexcept { return screen_; }

    /** @brief Number of frames rendered so far. */
    [[nodiscard]] int frame_count() const noexcept { return frame_count_; }

    /** @brief Current scroll position. */
    [[nodiscard]] int scroll_y() const noexcept { return scroll_y_; }
    void set_scroll_y(int y) { scroll_y_ = y; }

    /** @brief Viewport height (height - 2 for menu + status). */
    [[nodiscard]] int viewport_h() const noexcept { return static_cast<int>(cfg_.height) - 3; }

  private:
    config cfg_;
    virtual_screen screen_;
    content_fn content_fn_;
    status_fn status_fn_;
    int scroll_y_ = 0;
    int frame_count_ = 0;
};

} // namespace tapiru::testing
