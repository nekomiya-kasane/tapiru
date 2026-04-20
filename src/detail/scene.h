#pragma once

/**
 * @file scene.h
 * @brief The central data store for the widget tree (internal).
 *
 * SoA layout: topology arrays + per-type data pools.
 * Not a public header — only console, live, and builder .cpp files include this.
 */

#include <cassert>
#include <unordered_map>
#include <vector>

#include "detail/widget_types.h"
#include "tapiru/core/canvas.h"
#include "tapiru/core/style_table.h"
#include "tapiru/layout/types.h"

namespace tapiru::detail {

/** @brief Per-node flag bits stored in flags_ array. */
enum node_flags : uint8_t {
    flag_visible   = 0x01,  // bit 0: node is visible
    flag_focusable = 0x02,  // bit 1: node can receive focus/events
};

// ── Pool boilerplate macro ──────────────────────────────────────────────
// Generates: add_<name>(<Data> d), get_<name>(pool_index) (mut+const),
//            private vector member <name>s_, and clear entry.
#define TAPIRU_DEFINE_POOL_ACCESSORS(name, Data)                            \
    pool_index add_##name(Data d) {                                         \
        auto i = static_cast<pool_index>(name##s_.size());                  \
        name##s_.push_back(std::move(d));                                   \
        return i;                                                           \
    }                                                                       \
    [[nodiscard]] Data&       get_##name(pool_index i)       { return name##s_[i]; } \
    [[nodiscard]] const Data& get_##name(pool_index i) const { return name##s_[i]; }

#define TAPIRU_DEFINE_POOL_MEMBER(name, Data) \
    std::vector<Data> name##s_;

#define TAPIRU_DEFINE_POOL_CLEAR(name) name##s_.clear();

class scene {
public:
    // ── Node creation ───────────────────────────────────────────────────

    /** @brief Add a node and return its node_id. */
    node_id add_node(widget_type type, pool_index pidx, node_id parent = no_node,
                     widget_key key = no_key) {
        auto id = static_cast<node_id>(types_.size());
        types_.push_back(type);
        pool_indices_.push_back(pidx);
        parents_.push_back(parent);
        first_children_.push_back(no_node);
        next_siblings_.push_back(no_node);
        measurements_.push_back({});
        rects_.push_back({});
        z_orders_.push_back(0);
        flags_.push_back(flag_visible);
        keys_.push_back(key);
        prev_rects_.push_back({});

        // Link as child of parent
        if (parent != no_node) {
            if (first_children_[parent] == no_node) {
                first_children_[parent] = id;
            } else {
                // Append to end of sibling chain
                node_id sib = first_children_[parent];
                while (next_siblings_[sib] != no_node) {
                    sib = next_siblings_[sib];
                }
                next_siblings_[sib] = id;
            }
        }

        return id;
    }

    // ── Per-type pool accessors (macro-generated) ────────────────────────
    //
    // Each TAPIRU_DEFINE_POOL_ACCESSORS(name, Data) expands to:
    //   pool_index add_<name>(Data d);
    //   Data&       get_<name>(pool_index i);
    //   const Data& get_<name>(pool_index i) const;
    //
    // To add a new widget type, add one line here, one in the members
    // section, and one in clear(). See TAPIRU_POOL_LIST below.

    TAPIRU_DEFINE_POOL_ACCESSORS(text,          text_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(panel,         panel_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(rule,          rule_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(padding,       padding_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(columns,       columns_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(table,         table_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(overlay,       overlay_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(rows,          rows_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(menu,          menu_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(sized_box,     sized_box_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(center,        center_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(scroll_view,   scroll_view_data)
    TAPIRU_DEFINE_POOL_ACCESSORS(canvas_widget, canvas_widget_data)

    // ── Topology accessors ──────────────────────────────────────────────

    [[nodiscard]] size_t node_count() const noexcept { return types_.size(); }

    [[nodiscard]] widget_type type_of(node_id id)       const { return types_[id]; }
    [[nodiscard]] pool_index  pool_of(node_id id)       const { return pool_indices_[id]; }
    [[nodiscard]] node_id     parent_of(node_id id)     const { return parents_[id]; }
    [[nodiscard]] node_id     first_child(node_id id)   const { return first_children_[id]; }
    [[nodiscard]] node_id     next_sibling(node_id id)  const { return next_siblings_[id]; }

    // ── Layout results ──────────────────────────────────────────────────

    [[nodiscard]] measurement& meas(node_id id)       { return measurements_[id]; }
    [[nodiscard]] rect&        area(node_id id)       { return rects_[id]; }

    [[nodiscard]] const measurement& meas(node_id id) const { return measurements_[id]; }
    [[nodiscard]] const rect&        area(node_id id) const { return rects_[id]; }

    // ── Z-order & flags ────────────────────────────────────────────────

    [[nodiscard]] int16_t  z_order(node_id id) const { return z_orders_[id]; }
    void set_z_order(node_id id, int16_t z) { z_orders_[id] = z; }

    [[nodiscard]] uint8_t  flags(node_id id) const { return flags_[id]; }
    void set_flags(node_id id, uint8_t f) { flags_[id] = f; }

    [[nodiscard]] bool is_visible(node_id id)   const { return (flags_[id] & flag_visible) != 0; }
    [[nodiscard]] bool is_focusable(node_id id) const { return (flags_[id] & flag_focusable) != 0; }

    void set_visible(node_id id, bool v)   { if (v) flags_[id] |= flag_visible;   else flags_[id] &= ~flag_visible; }
    void set_focusable(node_id id, bool v) { if (v) flags_[id] |= flag_focusable; else flags_[id] &= ~flag_focusable; }

    // ── Widget keys ────────────────────────────────────────────────────

    [[nodiscard]] widget_key key_of(node_id id)  const { return keys_[id]; }
    void set_key(node_id id, widget_key k) { keys_[id] = k; }

    [[nodiscard]] rect&       prev_rect(node_id id)       { return prev_rects_[id]; }
    [[nodiscard]] const rect& prev_rect(node_id id) const { return prev_rects_[id]; }

    /** @brief Build key→node_id lookup map for reconciliation. */
    [[nodiscard]] std::unordered_map<widget_key, node_id> build_key_map() const {
        std::unordered_map<widget_key, node_id> m;
        for (node_id i = 0; i < static_cast<node_id>(keys_.size()); ++i) {
            if (keys_[i] != no_key) m[keys_[i]] = i;
        }
        return m;
    }

    // ── Style table ───────────────────────────────────────────────────

    [[nodiscard]] style_table& styles() noexcept { return styles_; }
    [[nodiscard]] const style_table& styles() const noexcept { return styles_; }

    // ── Capacity reuse ─────────────────────────────────────────────────

    /** @brief Clear all arrays without releasing capacity. For frame-to-frame reuse. */
    void clear() {
        types_.clear();
        pool_indices_.clear();
        parents_.clear();
        first_children_.clear();
        next_siblings_.clear();
        measurements_.clear();
        rects_.clear();
        z_orders_.clear();
        flags_.clear();
        keys_.clear();
        prev_rects_.clear();
        TAPIRU_DEFINE_POOL_CLEAR(text)
        TAPIRU_DEFINE_POOL_CLEAR(panel)
        TAPIRU_DEFINE_POOL_CLEAR(rule)
        TAPIRU_DEFINE_POOL_CLEAR(padding)
        TAPIRU_DEFINE_POOL_CLEAR(columns)
        TAPIRU_DEFINE_POOL_CLEAR(table)
        TAPIRU_DEFINE_POOL_CLEAR(overlay)
        TAPIRU_DEFINE_POOL_CLEAR(rows)
        TAPIRU_DEFINE_POOL_CLEAR(menu)
        TAPIRU_DEFINE_POOL_CLEAR(sized_box)
        TAPIRU_DEFINE_POOL_CLEAR(center)
        TAPIRU_DEFINE_POOL_CLEAR(scroll_view)
        TAPIRU_DEFINE_POOL_CLEAR(canvas_widget)
        styles_.clear();
    }

private:
    // Topology (SoA)
    std::vector<widget_type>  types_;
    std::vector<pool_index>   pool_indices_;
    std::vector<node_id>      parents_;
    std::vector<node_id>      first_children_;
    std::vector<node_id>      next_siblings_;

    // Layout results (SoA)
    std::vector<measurement>  measurements_;
    std::vector<rect>         rects_;

    // Metadata (SoA)
    std::vector<int16_t>      z_orders_;
    std::vector<uint8_t>      flags_;
    std::vector<widget_key>   keys_;
    std::vector<rect>         prev_rects_;

    // Per-type data pools (macro-generated members)
    TAPIRU_DEFINE_POOL_MEMBER(text,          text_data)
    TAPIRU_DEFINE_POOL_MEMBER(panel,         panel_data)
    TAPIRU_DEFINE_POOL_MEMBER(rule,          rule_data)
    TAPIRU_DEFINE_POOL_MEMBER(padding,       padding_data)
    TAPIRU_DEFINE_POOL_MEMBER(columns,       columns_data)
    TAPIRU_DEFINE_POOL_MEMBER(table,         table_data)
    TAPIRU_DEFINE_POOL_MEMBER(overlay,       overlay_data)
    TAPIRU_DEFINE_POOL_MEMBER(rows,          rows_data)
    TAPIRU_DEFINE_POOL_MEMBER(menu,          menu_data)
    TAPIRU_DEFINE_POOL_MEMBER(sized_box,     sized_box_data)
    TAPIRU_DEFINE_POOL_MEMBER(center,        center_data)
    TAPIRU_DEFINE_POOL_MEMBER(scroll_view,   scroll_view_data)
    TAPIRU_DEFINE_POOL_MEMBER(canvas_widget, canvas_widget_data)

    // Shared style table
    style_table styles_;
};

}  // namespace tapiru::detail

#undef TAPIRU_DEFINE_POOL_ACCESSORS
#undef TAPIRU_DEFINE_POOL_MEMBER
#undef TAPIRU_DEFINE_POOL_CLEAR
