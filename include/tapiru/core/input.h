#pragma once

/**
 * @file input.h
 * @brief Input event types (from tapioca) and event routing for interactive terminal UIs.
 *
 * Event types (key_event, mouse_event, resize_event, input_event, key_mod, special_key,
 * mouse_button, mouse_action, node_id) are defined in tapioca and re-exported here.
 * Event routing (event_table, focus_manager, hit_test) remains in tapiru.
 */

#include <functional>
#include <vector>

#include "tapioca/input.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

namespace tapiru {

// ── Re-export event types from tapioca ─────────────────────────────────

using tapioca::key_mod;
using tapioca::special_key;
using tapioca::key_event;
using tapioca::mouse_button;
using tapioca::mouse_action;
using tapioca::mouse_event;
using tapioca::resize_event;
using tapioca::input_event;
using tapioca::node_id;
using tapioca::key_action;
using tapioca::has_mod;

namespace detail { class scene; }

// ── Event callback types ───────────────────────────────────────────────

using key_handler   = std::function<bool(const key_event&)>;
using mouse_handler = std::function<bool(const mouse_event&)>;

// ── Event table ────────────────────────────────────────────────────────

/**
 * @brief Maps node_ids to event callbacks. Separate from widget data.
 */
class TAPIRU_API event_table {
public:
    void on_key(node_id id, key_handler handler);
    void on_mouse(node_id id, mouse_handler handler);

    /** @brief Dispatch a key event to the given node. Returns true if handled. */
    bool dispatch_key(node_id id, const key_event& ev) const;

    /** @brief Dispatch a mouse event to the given node. Returns true if handled. */
    bool dispatch_mouse(node_id id, const mouse_event& ev) const;

    void clear();

private:
    struct entry {
        node_id       id;
        key_handler   on_key;
        mouse_handler on_mouse;
    };
    std::vector<entry> entries_;

    entry* find(node_id id);
    const entry* find(node_id id) const;
};

// ── Focus manager ──────────────────────────────────────────────────────

/**
 * @brief Manages keyboard focus among focusable nodes.
 *
 * Maintains an ordered list of focusable node_ids and tracks the current focus.
 * Tab/Shift-Tab cycles through focusable nodes.
 */
class TAPIRU_API focus_manager {
public:
    void set_focusable_nodes(std::vector<node_id> nodes);

    [[nodiscard]] node_id focused() const noexcept { return focused_; }
    void focus(node_id id);
    void focus_next();
    void focus_prev();

    /** @brief Returns true if the given node currently has focus. */
    [[nodiscard]] bool has_focus(node_id id) const noexcept { return focused_ == id; }

private:
    std::vector<node_id> focusable_;
    node_id focused_ = UINT32_MAX;
};

// ── Hit testing ────────────────────────────────────────────────────────

/**
 * @brief Find the topmost focusable node at screen position (x, y).
 *
 * Iterates nodes in descending z_order, checking if (x,y) falls within
 * the node's rendered rect and the node has the focusable flag.
 */
TAPIRU_API node_id hit_test(const detail::scene& sc, uint32_t x, uint32_t y);

}  // namespace tapiru
