/**
 * @file grid.cpp
 * @brief grid_builder flatten_into implementation.
 */

#include "tapiru/widgets/grid.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

grid_builder &grid_builder::key(std::string_view k) {
    key_ = detail::fnv1a_hash(k);
    return *this;
}

node_id grid_builder::flatten_into(detail::scene &s) const {
    if (cells_.empty() || columns_ == 0) {
        return spacer_builder().flatten_into(s);
    }

    uint32_t num_rows = (static_cast<uint32_t>(cells_.size()) + columns_ - 1) / columns_;

    // Build as rows of columns
    detail::rows_data rd;
    rd.gap = row_gap_;
    auto rows_pi = s.add_rows(std::move(rd));
    auto rows_id = s.add_node(detail::widget_type::rows, rows_pi, detail::no_node, key_);
    if (z_order_ != 0) s.set_z_order(rows_id, z_order_);

    for (uint32_t r = 0; r < num_rows; ++r) {
        auto cols = columns_builder();
        cols.gap(col_gap_);

        for (uint32_t c = 0; c < columns_; ++c) {
            uint32_t idx = r * columns_ + c;
            if (idx < static_cast<uint32_t>(cells_.size()) && cells_[idx]) {
                // We need to flatten the cell content and wrap it
                // Since columns_builder uses type erasure, we create a wrapper
                // that just flattens the stored concept
                // Actually, we can't directly add a concept_ to columns_builder.
                // Instead, flatten each cell and add as a spacer-wrapped node.

                // Flatten cell directly into scene, then add to columns via manual scene manipulation
                // This requires building the columns manually.
                break; // Fall through to manual approach
            }
        }

        // Manual approach: build columns_data directly
        detail::columns_data cd;
        cd.gap = col_gap_;
        auto cols_pi = s.add_columns(std::move(cd));
        auto cols_id = s.add_node(detail::widget_type::columns, cols_pi, detail::no_node, 0);

        for (uint32_t c = 0; c < columns_; ++c) {
            uint32_t idx = r * columns_ + c;
            node_id cell_id;
            if (idx < static_cast<uint32_t>(cells_.size()) && cells_[idx]) {
                cell_id = cells_[idx]->flatten(s);
            } else {
                cell_id = spacer_builder().flatten_into(s);
            }
            s.get_columns(cols_pi).children.push_back(cell_id);
            s.get_columns(cols_pi).flex_ratios.push_back(1);
        }

        s.get_rows(rows_pi).children.push_back(cols_id);
        s.get_rows(rows_pi).flex_ratios.push_back(0);
    }

    return rows_id;
}

} // namespace tapiru
