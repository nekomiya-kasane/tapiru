/**
 * @file app_components.cpp
 * @brief Application-level component implementations: menu_bar, status_bar,
 *        resizable_split.
 */

#include "tapiru/widgets/app_components.h"

#include "tapiru/core/decorator.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/status_bar.h"

#include <algorithm>

namespace tapiru {

// ── menu_bar_component ──────────────────────────────────────────────────

namespace {

class menu_bar_component final : public component_base {
  public:
    menu_bar_component(std::vector<menu_bar_entry> entries, int *selected)
        : entries_(std::move(entries)), selected_(selected) {}

    element render() override {
        auto mb = menu_bar_builder();
        for (auto &e : entries_) {
            mb.add_menu(e.label, e.items);
        }
        mb.state(&state_);
        return element(std::move(mb));
    }

    bool on_event(const input_event &ev) override {
        if (!selected_) {
            return false;
        }
        if (auto *ke = std::get_if<key_event>(&ev)) {
            int n = static_cast<int>(entries_.size());
            if (n == 0) {
                return false;
            }
            if (ke->key == special_key::left) {
                state_.move_left(n);
                *selected_ = state_.active_index();
                return true;
            }
            if (ke->key == special_key::right) {
                state_.move_right(n);
                *selected_ = state_.active_index();
                return true;
            }
            if (ke->key == special_key::enter) {
                if (state_.is_open()) {
                    state_.deactivate();
                } else {
                    state_.activate(*selected_);
                }
                return true;
            }
            if (ke->key == special_key::escape) {
                state_.deactivate();
                return true;
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

  private:
    std::vector<menu_bar_entry> entries_;
    int *selected_;
    menu_bar_state state_;
};

// ── status_bar_component ────────────────────────────────────────────────

class status_bar_component final : public component_base {
  public:
    explicit status_bar_component(std::function<element()> fn) : fn_(std::move(fn)) {}

    element render() override { return fn_(); }

  private:
    std::function<element()> fn_;
};

// ── resizable_split_component ───────────────────────────────────────────

class resizable_split_component final : public component_base {
  public:
    resizable_split_component(component left, component right, int *split_pos, bool horizontal)
        : left_(std::move(left)), right_(std::move(right)), split_pos_(split_pos), horizontal_(horizontal) {
        // Default: left/top child is active
        active_ = 0;
    }

    element render() override {
        auto left_elem = left_ ? left_->render() : element{};
        auto right_elem = right_ ? right_->render() : element{};

        if (horizontal_) {
            auto cb = columns_builder();
            cb.add(std::move(left_elem));
            cb.add(std::move(right_elem));
            return element(std::move(cb));
        } else {
            auto rb = rows_builder();
            rb.add(std::move(left_elem));
            rb.add(std::move(right_elem));
            return element(std::move(rb));
        }
    }

    bool on_event(const input_event &ev) override {
        // Dispatch to active child first
        auto &active_child = (active_ == 0) ? left_ : right_;
        if (active_child && active_child->on_event(ev)) {
            return true;
        }

        if (auto *ke = std::get_if<key_event>(&ev)) {
            // Tab switches between left and right
            if (ke->key == special_key::tab) {
                active_ = 1 - active_;
                if (left_) {
                    left_->set_focused(active_ == 0);
                }
                if (right_) {
                    right_->set_focused(active_ == 1);
                }
                return true;
            }
            // Ctrl+Left/Right adjusts split position
            if (split_pos_ && has_mod(ke->mods, key_mod::ctrl)) {
                if ((horizontal_ && ke->key == special_key::left) || (!horizontal_ && ke->key == special_key::up)) {
                    *split_pos_ = std::max(1, *split_pos_ - 1);
                    return true;
                }
                if ((horizontal_ && ke->key == special_key::right) || (!horizontal_ && ke->key == special_key::down)) {
                    *split_pos_ = *split_pos_ + 1;
                    return true;
                }
            }
        }
        return false;
    }

    bool focusable() const override { return true; }

    std::vector<component> children() override {
        std::vector<component> kids;
        if (left_) {
            kids.push_back(left_);
        }
        if (right_) {
            kids.push_back(right_);
        }
        return kids;
    }

    component active_child() override { return (active_ == 0) ? left_ : right_; }

  private:
    component left_, right_;
    int *split_pos_;
    bool horizontal_;
    int active_ = 0;
};

} // anonymous namespace

// ── Factory functions ───────────────────────────────────────────────────

component make_menu_bar(std::vector<menu_bar_entry> entries, int *selected) {
    return std::make_shared<menu_bar_component>(std::move(entries), selected);
}

component make_status_bar(std::function<element()> content_fn) {
    return std::make_shared<status_bar_component>(std::move(content_fn));
}

component resizable_split_left(component left, component right, int *split_pos) {
    return std::make_shared<resizable_split_component>(std::move(left), std::move(right), split_pos, true);
}

component resizable_split_top(component top, component bottom, int *split_pos) {
    return std::make_shared<resizable_split_component>(std::move(top), std::move(bottom), split_pos, false);
}

} // namespace tapiru
