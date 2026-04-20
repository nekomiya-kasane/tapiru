#pragma once

/**
 * @file grid.h
 * @brief Grid layout widget — arranges children in a fixed-column grid.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <memory>
#include <string_view>
#include <vector>

namespace tapiru {

namespace detail {
class scene;
}
using node_id = uint32_t;

class TAPIRU_API grid_builder {
  public:
    grid_builder() = default;
    grid_builder(grid_builder &&) noexcept = default;
    grid_builder &operator=(grid_builder &&) noexcept = default;
    grid_builder(const grid_builder &) = delete;
    grid_builder &operator=(const grid_builder &) = delete;

    template <typename B> grid_builder &add(B &&child) {
        cells_.push_back(std::make_unique<model<std::decay_t<B>>>(std::forward<B>(child)));
        return *this;
    }

    grid_builder &columns(uint32_t c) {
        columns_ = c;
        return *this;
    }
    grid_builder &row_gap(uint32_t g) {
        row_gap_ = g;
        return *this;
    }
    grid_builder &col_gap(uint32_t g) {
        col_gap_ = g;
        return *this;
    }
    grid_builder &z_order(int16_t z) {
        z_order_ = z;
        return *this;
    }
    grid_builder &key(std::string_view k);

    node_id flatten_into(detail::scene &s) const;

  private:
    struct concept_ {
        virtual ~concept_() = default;
        virtual node_id flatten(detail::scene &s) const = 0;
    };
    template <typename B> struct model : concept_ {
        B builder;
        explicit model(B b) : builder(std::move(b)) {}
        node_id flatten(detail::scene &s) const override { return builder.flatten_into(s); }
    };

    std::vector<std::unique_ptr<concept_>> cells_;
    uint32_t columns_ = 2;
    uint32_t row_gap_ = 0;
    uint32_t col_gap_ = 1;
    int16_t z_order_ = 0;
    uint64_t key_ = 0;
};

} // namespace tapiru
