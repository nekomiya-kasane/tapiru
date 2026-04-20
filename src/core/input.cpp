/**
 * @file input.cpp
 * @brief Input event routing, focus management, and hit testing.
 */

#include "tapiru/core/input.h"
#include "detail/scene.h"

#include <algorithm>

namespace tapiru {

// ── event_table ────────────────────────────────────────────────────────

void event_table::on_key(node_id id, key_handler handler) {
    auto* e = find(id);
    if (e) {
        e->on_key = std::move(handler);
    } else {
        entries_.push_back({id, std::move(handler), {}});
    }
}

void event_table::on_mouse(node_id id, mouse_handler handler) {
    auto* e = find(id);
    if (e) {
        e->on_mouse = std::move(handler);
    } else {
        entries_.push_back({id, {}, std::move(handler)});
    }
}

bool event_table::dispatch_key(node_id id, const key_event& ev) const {
    auto* e = find(id);
    if (e && e->on_key) return e->on_key(ev);
    return false;
}

bool event_table::dispatch_mouse(node_id id, const mouse_event& ev) const {
    auto* e = find(id);
    if (e && e->on_mouse) return e->on_mouse(ev);
    return false;
}

void event_table::clear() {
    entries_.clear();
}

event_table::entry* event_table::find(node_id id) {
    for (auto& e : entries_) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

const event_table::entry* event_table::find(node_id id) const {
    for (const auto& e : entries_) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

// ── focus_manager ──────────────────────────────────────────────────────

void focus_manager::set_focusable_nodes(std::vector<node_id> nodes) {
    focusable_ = std::move(nodes);
    if (!focusable_.empty()) {
        // Keep current focus if still valid
        auto it = std::find(focusable_.begin(), focusable_.end(), focused_);
        if (it == focusable_.end()) {
            focused_ = focusable_.front();
        }
    } else {
        focused_ = UINT32_MAX;
    }
}

void focus_manager::focus(node_id id) {
    auto it = std::find(focusable_.begin(), focusable_.end(), id);
    if (it != focusable_.end()) {
        focused_ = id;
    }
}

void focus_manager::focus_next() {
    if (focusable_.empty()) return;
    auto it = std::find(focusable_.begin(), focusable_.end(), focused_);
    if (it == focusable_.end() || ++it == focusable_.end()) {
        focused_ = focusable_.front();
    } else {
        focused_ = *it;
    }
}

void focus_manager::focus_prev() {
    if (focusable_.empty()) return;
    auto it = std::find(focusable_.begin(), focusable_.end(), focused_);
    if (it == focusable_.end() || it == focusable_.begin()) {
        focused_ = focusable_.back();
    } else {
        --it;
        focused_ = *it;
    }
}

// ── Hit testing ────────────────────────────────────────────────────────

node_id hit_test(const detail::scene& sc, uint32_t x, uint32_t y) {
    // Build list of (z_order, node_id) for focusable nodes
    struct candidate {
        int16_t z;
        node_id id;
    };
    std::vector<candidate> candidates;

    uint32_t count = sc.node_count();
    for (uint32_t i = 0; i < count; ++i) {
        if (!sc.is_focusable(i)) continue;
        auto r = sc.area(i);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            candidates.push_back({sc.z_order(i), i});
        }
    }

    if (candidates.empty()) return UINT32_MAX;

    // Sort by z_order descending — highest z wins
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const candidate& a, const candidate& b) { return a.z > b.z; });

    return candidates.front().id;
}

}  // namespace tapiru
