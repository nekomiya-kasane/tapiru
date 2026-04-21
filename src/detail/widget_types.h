#pragma once

/**
 * @file widget_types.h
 * @brief Widget type enum and per-type data structs (internal).
 */

#include "tapiru/core/canvas.h"
#include "tapiru/core/gradient.h"
#include "tapiru/core/style.h"
#include "tapiru/core/style_table.h"
#include "tapiru/layout/types.h"
#include "tapiru/text/markup.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tapiru::detail {

    /** @brief Shader callback: post-processes a region of the canvas. */
    using shader_fn = std::function<void(canvas &cv, style_table &styles, rect region, double frame_time)>;

    using node_id = uint32_t;
    using pool_index = uint32_t;
    using widget_key = uint64_t;

    inline constexpr node_id no_node = UINT32_MAX;
    inline constexpr pool_index no_pool = UINT32_MAX;
    inline constexpr widget_key no_key = 0;

    enum class widget_type : uint8_t {
        text,
        panel,
        rule,
        padding,
        columns,
        table,
        overlay,
        rows,
        spacer,
        menu,
        sized_box,
        center,
        scroll_view,
        canvas_widget,
        count_
    };

    // ── Per-type data structs ───────────────────────────────────────────────

    struct text_data {
        std::vector<text_fragment> fragments;
        overflow_mode overflow = overflow_mode::wrap;
        justify align = justify::left;
    };

    /** @brief Shadow/glow configuration for panels. */
    struct shadow_config {
        int8_t offset_x = 2; // cells (wider default for ~2:1 terminal aspect ratio)
        int8_t offset_y = 1; // cells
        uint8_t blur = 1;    // 0=hard, 1-2=soft
        color shadow_color = color::from_rgb(0, 0, 0);
        uint8_t opacity = 128; // shadow alpha
    };

    struct panel_data {
        node_id content = no_node;
        border_style border = border_style::rounded;
        style border_sty;
        std::string title;
        style title_sty;
        uint8_t alpha = 255; // 0=transparent, 255=opaque
        std::optional<linear_gradient> bg_gradient;
        std::optional<shadow_config> shadow;
        shader_fn shader; // post-process callback (may be empty)
    };

    struct rule_data {
        std::string title;
        style rule_sty;
        justify align = justify::center;
        char32_t ch = U'\x2500'; // ─
        std::optional<linear_gradient> gradient;
    };

    struct padding_data {
        uint8_t top = 0;
        uint8_t right = 0;
        uint8_t bottom = 0;
        uint8_t left = 0;
        node_id content = no_node;
    };

    struct columns_data {
        std::vector<node_id> children;
        std::vector<uint32_t> flex_ratios; // 0 = auto-size, >0 = flex weight
        uint32_t gap = 1;
        uint8_t alpha = 255;
        bool wrap = false;
        justify justify_content = justify::left;
        align_items align = align_items::stretch;
    };

    struct table_column_def {
        std::string header;
        justify align = justify::left;
        uint32_t min_width = 0;
        uint32_t max_width = unbounded;
    };

    struct table_data {
        std::vector<table_column_def> column_defs;
        std::vector<std::vector<text_fragment>> cells; // row-major: [row * ncols + col]
        border_style border = border_style::rounded;
        style border_sty;
        style header_sty;
        bool show_header = true;
        std::optional<shadow_config> shadow;
        std::optional<linear_gradient> border_gradient;
    };

    struct overlay_data {
        node_id base = no_node;
        node_id overlay = no_node;
        uint8_t alpha = 255;
    };

    struct rows_data {
        std::vector<node_id> children;
        std::vector<uint32_t> flex_ratios; // 0 = auto-size, >0 = flex weight
        uint32_t gap = 0;
        uint8_t alpha = 255;
        bool wrap = false;
        justify justify_content = justify::left;
        align_items align = align_items::stretch;
    };

    struct sized_box_data {
        node_id content = no_node;
        uint32_t fixed_w = 0; // 0 = unconstrained
        uint32_t fixed_h = 0; // 0 = unconstrained
        uint32_t min_w = 0;
        uint32_t min_h = 0;
        uint32_t max_w = UINT32_MAX;
        uint32_t max_h = UINT32_MAX;
    };

    struct center_data {
        node_id content = no_node;
        bool horizontal = true;
        bool vertical = true;
    };

    struct scroll_view_data {
        node_id content = no_node;
        const int *scroll_x = nullptr; // external scroll offset (read-only)
        const int *scroll_y = nullptr; // external scroll offset (read-only)
        uint32_t view_w = 0;           // viewport width  (0 = fill parent)
        uint32_t view_h = 0;           // viewport height (0 = fill parent)
        bool show_scrollbar_v = true;
        bool show_scrollbar_h = false;
        border_style border = border_style::none;
        style border_sty;
    };

    struct menu_item {
        std::string label;
        std::string shortcut; // "" = no shortcut
        bool separator = false;
        bool disabled = false;
        bool checked = false;
        std::string icon;                // "" = no icon (e.g. emoji or single char)
        std::vector<menu_item> children; // non-empty = has submenu

        [[nodiscard]] bool has_submenu() const noexcept { return !children.empty(); }
    };

    struct canvas_pixel {
        int x = 0;
        int y = 0;
        color c;
    };

    struct canvas_line {
        int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        color c;
        bool block_mode = false;
    };

    struct canvas_circle {
        int cx = 0, cy = 0, r = 0;
        color c;
    };

    struct canvas_rect_cmd {
        int x = 0, y = 0, w = 0, h = 0;
        color c;
    };

    struct canvas_text_cmd {
        int x = 0, y = 0;
        std::string text;
        style sty;
    };

    struct canvas_widget_data {
        uint32_t pixel_w = 0;
        uint32_t pixel_h = 0;
        std::vector<canvas_pixel> points;
        std::vector<canvas_line> lines;
        std::vector<canvas_circle> circles;
        std::vector<canvas_rect_cmd> rects;
        std::vector<canvas_text_cmd> texts;
    };

    struct menu_data {
        std::vector<menu_item> items;
        const int *cursor = nullptr; // external cursor state (read-only)
        border_style border = border_style::rounded;
        style border_sty;
        style highlight_sty; // selected item style
        style shortcut_sty;  // shortcut text style
        style item_sty;      // normal item style
        style disabled_sty;  // disabled item style
        std::optional<shadow_config> shadow;

        // Multi-level state (read-only pointers to external state)
        const std::vector<int> *open_path = nullptr; // stack of cursor indices per depth
    };

} // namespace tapiru::detail
