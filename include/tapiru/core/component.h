#pragma once

/**
 * @file component.h
 * @brief Interactive component abstraction: render + event handling + focus.
 *
 * A component is the sole interactive unit in tapiru. It produces an element
 * on each frame, handles input events, and participates in focus management.
 *
 * Usage:
 *   auto btn = make_button(button_option{ .label = "OK", .on_click = []{} });
 *   auto renderer = make_renderer([](bool focused) { return text_builder("hi"); });
 *   auto container = container::vertical({ btn, renderer });
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include "tapiru/core/element.h"
#include "tapiru/core/input.h"
#include "tapiru/exports.h"

namespace tapiru {

// ── component_base ──────────────────────────────────────────────────────

class TAPIRU_API component_base {
public:
    virtual ~component_base() = default;

    /** @brief Produce the element tree for the current frame. */
    virtual element render() = 0;

    /** @brief Handle an input event. Return true if consumed. */
    virtual bool on_event(const input_event& ev) { (void)ev; return false; }

    /** @brief Whether this component can receive focus. */
    [[nodiscard]] virtual bool focusable() const { return false; }

    /** @brief Whether this component currently has focus. */
    [[nodiscard]] bool focused() const noexcept { return focused_; }

    /** @brief Set focus state (called by the screen/container). */
    void set_focused(bool f) noexcept { focused_ = f; }

    /** @brief Called when the component gains focus. */
    virtual void on_focus() {}

    /** @brief Called when the component loses focus. */
    virtual void on_blur() {}

    /** @brief Get child components for container traversal. */
    [[nodiscard]] virtual std::vector<std::shared_ptr<component_base>> children() { return {}; }

    /** @brief Get the currently active/focused child (for containers). */
    [[nodiscard]] virtual std::shared_ptr<component_base> active_child() { return nullptr; }

private:
    bool focused_ = false;
};

// ── component (shared_ptr handle) ───────────────────────────────────────

using component = std::shared_ptr<component_base>;

// ── make_renderer ───────────────────────────────────────────────────────

TAPIRU_API component make_renderer(std::function<element(bool focused)> render_fn);
TAPIRU_API component make_renderer(std::function<element()> render_fn);

// ── container ───────────────────────────────────────────────────────────

namespace container {

TAPIRU_API component vertical(std::vector<component> children);
TAPIRU_API component horizontal(std::vector<component> children);
TAPIRU_API component tab(std::vector<component> children, int* selected = nullptr);
TAPIRU_API component stacked(std::vector<component> children, int* selected = nullptr);

}  // namespace container

// ── catch_event ─────────────────────────────────────────────────────────

TAPIRU_API component catch_event(component inner, std::function<bool(const input_event&)> handler);

}  // namespace tapiru
