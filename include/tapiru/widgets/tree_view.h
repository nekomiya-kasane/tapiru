#pragma once

/**
 * @file tree_view.h
 * @brief Tree view widget — hierarchical collapsible tree display.
 */

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

struct tree_node {
    std::string label;
    std::vector<tree_node> children;
};

class TAPIRU_API tree_view_builder {
public:
    tree_view_builder() = default;

    tree_view_builder& root(tree_node r)                           { root_ = std::move(r); return *this; }
    tree_view_builder& expanded_set(const std::unordered_set<std::string>* s) { expanded_ = s; return *this; }
    tree_view_builder& cursor(const int* c)                        { cursor_ = c; return *this; }
    tree_view_builder& node_style(const style& s)                  { node_sty_ = s; return *this; }
    tree_view_builder& highlight_style(const style& s)             { highlight_sty_ = s; return *this; }
    tree_view_builder& guide_style(const style& s)                 { guide_sty_ = s; return *this; }
    tree_view_builder& z_order(int16_t z)                          { z_order_ = z; return *this; }
    tree_view_builder& key(std::string_view k);

    node_id flatten_into(detail::scene& s) const;

private:
    tree_node root_;
    const std::unordered_set<std::string>* expanded_ = nullptr;
    const int* cursor_ = nullptr;
    style node_sty_;
    style highlight_sty_;
    style guide_sty_;
    int16_t z_order_ = 0;
    uint64_t key_ = 0;
};

}  // namespace tapiru
