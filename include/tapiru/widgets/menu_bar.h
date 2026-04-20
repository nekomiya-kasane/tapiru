#pragma once

/**
 * @file menu_bar.h
 * @brief Horizontal menu bar widget — a row of top-level labels that open dropdown menus.
 *
 * Composes columns_builder + text_builder for the bar, and menu_builder for dropdowns.
 * State is managed externally via menu_bar_state.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"
#include "tapiru/widgets/menu.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

namespace detail {
class scene;
}
using node_id = uint32_t;

// ── menu_bar_state — tracks which top-level item is active ───────────

class TAPIRU_API menu_bar_state {
  public:
    menu_bar_state() = default;

    [[nodiscard]] int active_index() const noexcept { return active_; }
    [[nodiscard]] bool is_open() const noexcept { return open_; }
    [[nodiscard]] int selected_item() const noexcept { return selected_; }
    [[nodiscard]] bool was_selected() const noexcept { return was_selected_; }

    void activate(int index) {
        active_ = index;
        open_ = true;
    }
    void deactivate() { open_ = false; }
    void move_left(int count) {
        if (count > 0) active_ = (active_ - 1 + count) % count;
    }
    void move_right(int count) {
        if (count > 0) active_ = (active_ + 1) % count;
    }

    void set_selected(int global_index) {
        selected_ = global_index;
        was_selected_ = true;
    }
    void clear_selection() {
        selected_ = -1;
        was_selected_ = false;
    }

    menu_state &dropdown_state() noexcept { return dropdown_; }
    [[nodiscard]] const menu_state &dropdown_state() const noexcept { return dropdown_; }

  private:
    int active_ = 0;
    bool open_ = false;
    int selected_ = -1;
    bool was_selected_ = false;
    menu_state dropdown_;
};

// ── menu_bar_entry — a single top-level menu with its dropdown items ─

struct menu_bar_entry {
    std::string label;
    std::vector<menu_item_builder> items;
};

// ── menu_bar_builder — fluent builder for the horizontal menu bar ────

class TAPIRU_API menu_bar_builder {
  public:
    menu_bar_builder() = default;

    menu_bar_builder &add_menu(std::string_view label, std::vector<menu_item_builder> items) {
        entries_.push_back({std::string(label), std::move(items)});
        return *this;
    }

    menu_bar_builder &state(const menu_bar_state *s) {
        state_ = s;
        return *this;
    }
    menu_bar_builder &bar_style(const style &s) {
        bar_sty_ = s;
        return *this;
    }
    menu_bar_builder &active_style(const style &s) {
        active_sty_ = s;
        return *this;
    }
    menu_bar_builder &highlight_style(const style &s) {
        highlight_sty_ = s;
        return *this;
    }
    menu_bar_builder &shortcut_style(const style &s) {
        shortcut_sty_ = s;
        return *this;
    }
    menu_bar_builder &item_style(const style &s) {
        item_sty_ = s;
        return *this;
    }
    menu_bar_builder &dropdown_border(border_style bs) {
        dropdown_border_ = bs;
        return *this;
    }
    menu_bar_builder &z_order(int16_t z) {
        z_order_ = z;
        return *this;
    }
    menu_bar_builder &key(std::string_view k);

    node_id flatten_into(detail::scene &s) const;

  private:
    std::vector<menu_bar_entry> entries_;
    const menu_bar_state *state_ = nullptr;
    style bar_sty_;
    style active_sty_;
    style highlight_sty_;
    style shortcut_sty_;
    style item_sty_;
    border_style dropdown_border_ = border_style::rounded;
    int16_t z_order_ = 0;
    uint64_t key_ = 0;
};

} // namespace tapiru
