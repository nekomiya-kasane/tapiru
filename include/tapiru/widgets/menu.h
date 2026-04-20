#pragma once

/**
 * @file menu.h
 * @brief Menu widget — a bordered dropdown/context menu with selectable items.
 *
 * Supports multi-level submenus via menu_item_builder, menu_tree, and menu_state.
 */

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

// ── menu_item_builder — fluent builder for a single menu item ────────

class TAPIRU_API menu_item_builder {
public:
    explicit menu_item_builder(std::string_view label) : label_(label) {}

    menu_item_builder& shortcut(std::string_view s)  { shortcut_ = s; return *this; }
    menu_item_builder& disabled(bool v = true)       { disabled_ = v; return *this; }
    menu_item_builder& checked(bool v = true)        { checked_ = v; return *this; }
    menu_item_builder& icon(std::string_view i)      { icon_ = i; return *this; }

    menu_item_builder& submenu(std::function<void(menu_item_builder&)> fn) {
        menu_item_builder sub("");
        fn(sub);
        children_ = std::move(sub.children_);
        return *this;
    }

    menu_item_builder& add(menu_item_builder item) {
        children_.push_back(std::move(item));
        return *this;
    }

    menu_item_builder& add_separator() {
        menu_item_builder sep("");
        sep.separator_ = true;
        children_.push_back(std::move(sep));
        return *this;
    }

    /** @brief Create a separator pseudo-item (for use in item lists). */
    static menu_item_builder separator() {
        menu_item_builder sep("");
        sep.separator_ = true;
        return sep;
    }

private:
    friend class menu_builder;
    friend class menu_tree;

    std::string label_;
    std::string shortcut_;
    bool        separator_ = false;
    bool        disabled_  = false;
    bool        checked_   = false;
    std::string icon_;
    std::vector<menu_item_builder> children_;
};

// ── menu_tree — read-only tree for state queries ─────────────────────

class TAPIRU_API menu_tree {
public:
    struct node {
        std::string label;
        std::string shortcut;
        bool        separator = false;
        bool        disabled  = false;
        bool        checked   = false;
        std::string icon;
        std::vector<node> children;
        int         global_index = -1;  // -1 for separators

        [[nodiscard]] bool has_submenu() const noexcept { return !children.empty(); }
        [[nodiscard]] bool is_selectable() const noexcept { return !separator && !disabled; }
    };

    menu_tree() = default;

    void add_item(const menu_item_builder& b);
    void add_separator();

    [[nodiscard]] const std::vector<node>& roots() const noexcept { return roots_; }
    [[nodiscard]] int total_items() const noexcept { return next_index_; }

    [[nodiscard]] const std::vector<node>& children_at(const std::vector<int>& path) const;

    [[nodiscard]] int next_selectable(const std::vector<node>& items, int from, int dir) const;

private:
    void build_node(node& n, const menu_item_builder& b);
    std::vector<node> roots_;
    int next_index_ = 0;
};

// ── menu_state — multi-level cursor + safe triangle + timers ─────────

class TAPIRU_API menu_state {
public:
    menu_state();

    // ── Query ──
    [[nodiscard]] int  cursor() const;
    [[nodiscard]] int  cursor_at(int depth) const;
    [[nodiscard]] int  depth() const;
    [[nodiscard]] bool is_submenu_open() const;
    [[nodiscard]] int  selected_item() const;
    [[nodiscard]] bool was_selected() const;
    [[nodiscard]] bool is_keyboard_mode() const;
    [[nodiscard]] const std::vector<int>& open_path() const noexcept { return path_; }

    // ── Keyboard operations ──
    void move_up(const menu_tree& tree);
    void move_down(const menu_tree& tree);
    void move_home(const menu_tree& tree);
    void move_end(const menu_tree& tree);
    void open_submenu(const menu_tree& tree);
    void close_submenu();
    void select(const menu_tree& tree);
    void type_char(char32_t ch, const menu_tree& tree);

    // ── Mouse operations ──
    void hover(int depth, int item_index, int mouse_x, int mouse_y,
               const menu_tree& tree);
    void click(int depth, int item_index, const menu_tree& tree);
    void mouse_leave();
    void tick(std::chrono::milliseconds dt);

    // ── Reset ──
    void clear_selection();
    void close_all();

    // ── Configuration ──
    void set_open_delay(std::chrono::milliseconds ms)  { open_delay_ = ms; }
    void set_close_delay(std::chrono::milliseconds ms) { close_delay_ = ms; }
    void set_leave_delay(std::chrono::milliseconds ms) { leave_delay_ = ms; }

    // ── Safe triangle ──
    void set_submenu_rect(int depth, rect r);
    [[nodiscard]] bool is_in_safe_triangle(int mouse_x, int mouse_y) const;

private:
    std::vector<int> path_;  // cursor index at each depth (path_[0] = top-level cursor)
    int  selected_     = -1;
    bool was_selected_ = false;
    bool keyboard_mode_ = false;

    // Safe triangle
    int safe_px_ = 0, safe_py_ = 0;  // last hover point when submenu opened
    rect submenu_rect_;               // current deepest submenu rect

    // Timers
    int pending_open_item_ = -1;
    std::chrono::milliseconds open_timer_{0};
    std::chrono::milliseconds close_timer_{0};
    std::chrono::milliseconds leave_timer_{0};
    bool close_pending_ = false;
    bool leave_pending_ = false;

    // Configuration
    std::chrono::milliseconds open_delay_{150};
    std::chrono::milliseconds close_delay_{100};
    std::chrono::milliseconds leave_delay_{300};

    const std::vector<menu_tree::node>& items_at_depth(const menu_tree& tree, int d) const;
};

// ── menu_builder — extended to support multi-level menus ─────────────

class TAPIRU_API menu_builder {
public:
    menu_builder() = default;

    // Legacy flat API (backward compatible)
    menu_builder& add_item(std::string_view label, std::string_view shortcut = "") {
        items_.push_back(menu_item_builder(label).shortcut(shortcut));
        return *this;
    }

    menu_builder& add_separator() {
        menu_item_builder sep("");
        sep.separator_ = true;
        items_.push_back(std::move(sep));
        return *this;
    }

    // New multi-level API
    menu_builder& add(menu_item_builder item) {
        items_.push_back(std::move(item));
        return *this;
    }

    menu_builder& cursor(const int* c)                   { cursor_ = c; return *this; }
    menu_builder& state(const menu_state* s)             { state_ = s; return *this; }
    menu_builder& border(border_style bs)                { border_ = bs; return *this; }
    menu_builder& border_style_override(const style& s)  { border_sty_ = s; return *this; }
    menu_builder& highlight_style(const style& s)        { highlight_sty_ = s; return *this; }
    menu_builder& shortcut_style(const style& s)         { shortcut_sty_ = s; return *this; }
    menu_builder& item_style(const style& s)             { item_sty_ = s; return *this; }
    menu_builder& disabled_style(const style& s)         { disabled_sty_ = s; return *this; }
    menu_builder& z_order(int16_t z)                     { z_order_ = z; return *this; }
    menu_builder& key(std::string_view k);

    menu_builder& shadow(int8_t offset_x = 2, int8_t offset_y = 1, uint8_t blur = 1,
                         color shadow_color = color::from_rgb(0,0,0), uint8_t opacity = 128) {
        shadow_ = {offset_x, offset_y, blur, shadow_color, opacity};
        return *this;
    }
    menu_builder& glow(color glow_color, uint8_t blur = 1, uint8_t opacity = 100) {
        shadow_ = {0, 0, blur, glow_color, opacity};
        return *this;
    }

    node_id flatten_into(detail::scene& s) const;

private:
    struct shadow_cfg {
        int8_t  offset_x = 2;
        int8_t  offset_y = 1;
        uint8_t blur     = 1;
        color   shadow_color = color::from_rgb(0,0,0);
        uint8_t opacity  = 128;
    };

    std::vector<menu_item_builder> items_;
    const int*         cursor_        = nullptr;
    const menu_state*  state_         = nullptr;
    border_style       border_        = border_style::rounded;
    style              border_sty_;
    style              highlight_sty_;
    style              shortcut_sty_;
    style              item_sty_;
    style              disabled_sty_;
    int16_t            z_order_       = 0;
    uint64_t           key_           = 0;
    std::optional<shadow_cfg> shadow_;
};

}  // namespace tapiru
