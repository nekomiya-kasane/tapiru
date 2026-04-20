/**
 * @file component.cpp
 * @brief Component implementations: renderer, containers, catch_event.
 */

#include "tapiru/core/component.h"

#include "tapiru/core/decorator.h"
#include "tapiru/widgets/builders.h"

#include <algorithm>
#include <cassert>

namespace tapiru {

// ── renderer_component ──────────────────────────────────────────────────

namespace {

class renderer_component final : public component_base {
  public:
    explicit renderer_component(std::function<element(bool)> fn) : fn_(std::move(fn)) {}

    element render() override { return fn_(focused()); }

  private:
    std::function<element(bool)> fn_;
};

class simple_renderer_component final : public component_base {
  public:
    explicit simple_renderer_component(std::function<element()> fn) : fn_(std::move(fn)) {}

    element render() override { return fn_(); }

  private:
    std::function<element()> fn_;
};

} // anonymous namespace

component make_renderer(std::function<element(bool focused)> render_fn) {
    return std::make_shared<renderer_component>(std::move(render_fn));
}

component make_renderer(std::function<element()> render_fn) {
    return std::make_shared<simple_renderer_component>(std::move(render_fn));
}

// ── container_base ──────────────────────────────────────────────────────

namespace {

class container_component : public component_base {
  public:
    explicit container_component(std::vector<component> kids, bool vertical)
        : children_(std::move(kids)), vertical_(vertical) {
        // Find first focusable child
        for (size_t i = 0; i < children_.size(); ++i) {
            if (children_[i] && (children_[i]->focusable() || !children_[i]->children().empty())) {
                active_ = static_cast<int>(i);
                break;
            }
        }
    }

    element render() override {
        if (vertical_) {
            auto rb = rows_builder();
            for (auto &child : children_) {
                if (child) rb.add(child->render());
            }
            return element(std::move(rb));
        } else {
            auto cb = columns_builder();
            for (auto &child : children_) {
                if (child) cb.add(child->render());
            }
            return element(std::move(cb));
        }
    }

    bool on_event(const input_event &ev) override {
        // First, try to dispatch to the active child
        if (active_ >= 0 && active_ < static_cast<int>(children_.size())) {
            if (children_[active_]->on_event(ev)) return true;
        }

        // Handle Tab/Shift-Tab for focus cycling
        if (auto *ke = std::get_if<key_event>(&ev)) {
            if (ke->key == special_key::tab) {
                if (has_mod(ke->mods, key_mod::shift)) {
                    return focus_prev_child();
                } else {
                    return focus_next_child();
                }
            }
        }
        return false;
    }

    bool focusable() const override {
        return std::any_of(children_.begin(), children_.end(),
                           [](const component &c) { return c && (c->focusable() || !c->children().empty()); });
    }

    std::vector<component> children() override { return children_; }

    component active_child() override {
        if (active_ >= 0 && active_ < static_cast<int>(children_.size())) return children_[active_];
        return nullptr;
    }

  private:
    bool focus_next_child() {
        int start = active_;
        int n = static_cast<int>(children_.size());
        for (int i = 1; i <= n; ++i) {
            int idx = (start + i) % n;
            if (children_[idx] && (children_[idx]->focusable() || !children_[idx]->children().empty())) {
                set_active(idx);
                return true;
            }
        }
        return false;
    }

    bool focus_prev_child() {
        int start = active_;
        int n = static_cast<int>(children_.size());
        for (int i = 1; i <= n; ++i) {
            int idx = (start - i + n) % n;
            if (children_[idx] && (children_[idx]->focusable() || !children_[idx]->children().empty())) {
                set_active(idx);
                return true;
            }
        }
        return false;
    }

    void set_active(int idx) {
        if (active_ >= 0 && active_ < static_cast<int>(children_.size())) {
            children_[active_]->set_focused(false);
            children_[active_]->on_blur();
        }
        active_ = idx;
        if (active_ >= 0 && active_ < static_cast<int>(children_.size())) {
            children_[active_]->set_focused(true);
            children_[active_]->on_focus();
        }
    }

    std::vector<component> children_;
    bool vertical_;
    int active_ = -1;
};

// ── tab_container ───────────────────────────────────────────────────────

class tab_container : public component_base {
  public:
    tab_container(std::vector<component> kids, int *selected) : children_(std::move(kids)), selected_(selected) {}

    element render() override {
        int idx = selected_ ? *selected_ : 0;
        if (idx >= 0 && idx < static_cast<int>(children_.size()) && children_[idx]) return children_[idx]->render();
        return element{};
    }

    bool on_event(const input_event &ev) override {
        int idx = selected_ ? *selected_ : 0;
        if (idx >= 0 && idx < static_cast<int>(children_.size()) && children_[idx]) return children_[idx]->on_event(ev);
        return false;
    }

    bool focusable() const override { return true; }
    std::vector<component> children() override { return children_; }

    component active_child() override {
        int idx = selected_ ? *selected_ : 0;
        if (idx >= 0 && idx < static_cast<int>(children_.size())) return children_[idx];
        return nullptr;
    }

  private:
    std::vector<component> children_;
    int *selected_;
};

// ── stacked_container ───────────────────────────────────────────────────

class stacked_container : public component_base {
  public:
    stacked_container(std::vector<component> kids, int *selected) : children_(std::move(kids)), selected_(selected) {}

    element render() override {
        // Render all children stacked, with the selected one on top
        if (children_.empty()) return element{};
        int idx = selected_ ? *selected_ : 0;
        if (idx < 0 || idx >= static_cast<int>(children_.size())) idx = 0;
        if (children_[idx]) return children_[idx]->render();
        return element{};
    }

    bool on_event(const input_event &ev) override {
        int idx = selected_ ? *selected_ : 0;
        if (idx >= 0 && idx < static_cast<int>(children_.size()) && children_[idx]) return children_[idx]->on_event(ev);
        return false;
    }

    bool focusable() const override { return true; }
    std::vector<component> children() override { return children_; }

    component active_child() override {
        int idx = selected_ ? *selected_ : 0;
        if (idx >= 0 && idx < static_cast<int>(children_.size())) return children_[idx];
        return nullptr;
    }

  private:
    std::vector<component> children_;
    int *selected_;
};

// ── catch_event_component ───────────────────────────────────────────────

class catch_event_component final : public component_base {
  public:
    catch_event_component(component inner, std::function<bool(const input_event &)> handler)
        : inner_(std::move(inner)), handler_(std::move(handler)) {}

    element render() override { return inner_->render(); }

    bool on_event(const input_event &ev) override {
        if (handler_(ev)) return true;
        return inner_->on_event(ev);
    }

    bool focusable() const override { return inner_->focusable(); }
    std::vector<component> children() override { return {inner_}; }
    component active_child() override { return inner_; }

  private:
    component inner_;
    std::function<bool(const input_event &)> handler_;
};

} // anonymous namespace

// ── container factories ─────────────────────────────────────────────────

namespace container {

component vertical(std::vector<component> children) {
    return std::make_shared<container_component>(std::move(children), true);
}

component horizontal(std::vector<component> children) {
    return std::make_shared<container_component>(std::move(children), false);
}

component tab(std::vector<component> children, int *selected) {
    return std::make_shared<tab_container>(std::move(children), selected);
}

component stacked(std::vector<component> children, int *selected) {
    return std::make_shared<stacked_container>(std::move(children), selected);
}

} // namespace container

// ── catch_event ─────────────────────────────────────────────────────────

component catch_event(component inner, std::function<bool(const input_event &)> handler) {
    return std::make_shared<catch_event_component>(std::move(inner), std::move(handler));
}

} // namespace tapiru
