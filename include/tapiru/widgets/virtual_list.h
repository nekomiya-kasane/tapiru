#pragma once

/**
 * @file virtual_list.h
 * @brief Virtual list widget: only measures/renders visible items.
 *
 * Usage:
 *   virtual_list_builder(1000, 10)
 *       .item_builder([](uint32_t idx) { return text_builder("Item " + std::to_string(idx)); })
 *       .scroll_offset(scroll_pos)
 */

#include <cstdint>
#include <functional>

#include "tapiru/exports.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

/**
 * @brief Builder for a virtualized list that only constructs visible items.
 *
 * The item_builder callback is invoked only for items in the visible window
 * [scroll_offset, scroll_offset + visible_height).
 */
class TAPIRU_API virtual_list_builder {
public:
    using item_fn = std::function<text_builder(uint32_t index)>;

    virtual_list_builder(uint32_t total_items, uint32_t visible_height)
        : total_(total_items), visible_(visible_height) {}

    virtual_list_builder& item_builder(item_fn fn) { item_fn_ = std::move(fn); return *this; }
    virtual_list_builder& scroll_offset(uint32_t offset) { offset_ = offset; return *this; }
    virtual_list_builder& key(std::string_view k) { key_ = detail::fnv1a_hash(k); return *this; }
    virtual_list_builder& z_order(int16_t z) { z_order_ = z; return *this; }

    node_id flatten_into(detail::scene& s) const;

    [[nodiscard]] uint32_t total_items() const noexcept { return total_; }
    [[nodiscard]] uint32_t visible_height() const noexcept { return visible_; }
    [[nodiscard]] uint32_t offset() const noexcept { return offset_; }

private:
    uint32_t total_   = 0;
    uint32_t visible_ = 0;
    uint32_t offset_  = 0;
    item_fn  item_fn_;
    uint64_t key_     = 0;
    int16_t  z_order_ = 0;
};

}  // namespace tapiru
