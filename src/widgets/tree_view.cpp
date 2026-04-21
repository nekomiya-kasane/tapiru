/**
 * @file tree_view.cpp
 * @brief tree_view_builder flatten_into implementation.
 */

#include "tapiru/widgets/tree_view.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

namespace tapiru {

    tree_view_builder &tree_view_builder::key(std::string_view k) {
        key_ = detail::fnv1a_hash(k);
        return *this;
    }

    namespace {

        struct flatten_ctx {
            detail::scene &sc;
            detail::pool_index rows_pi;
            const std::unordered_set<std::string> *expanded;
            const int *cursor;
            const style &node_sty;
            const style &highlight_sty;
        };

        void flatten_node(flatten_ctx &ctx, const tree_node &node, int depth, int &flat_index) {
            int cur = ctx.cursor ? *ctx.cursor : -1;
            bool is_selected = (flat_index == cur);
            bool has_children = !node.children.empty();
            bool is_expanded = ctx.expanded && ctx.expanded->count(node.label) > 0;

            std::string line;
            for (int i = 0; i < depth; ++i) {
                line += "  ";
            }
            if (has_children) {
                line += is_expanded ? "\xe2\x96\xbe " : "\xe2\x96\xb8 ";
            } else {
                line += "  ";
            }
            line += node.label;

            auto tb = text_builder(line);
            if (is_selected) {
                tb.style_override(ctx.highlight_sty);
            } else {
                tb.style_override(ctx.node_sty);
            }

            auto text_id = tb.flatten_into(ctx.sc);
            ctx.sc.get_rows(ctx.rows_pi).children.push_back(text_id);
            ctx.sc.get_rows(ctx.rows_pi).flex_ratios.push_back(0);
            flat_index++;

            if (has_children && is_expanded) {
                for (const auto &child : node.children) {
                    flatten_node(ctx, child, depth + 1, flat_index);
                }
            }
        }

    } // anonymous namespace

    node_id tree_view_builder::flatten_into(detail::scene &s) const {
        detail::rows_data rd;
        rd.gap = 0;

        auto pi = s.add_rows(std::move(rd));
        auto rows_id = s.add_node(detail::widget_type::rows, pi, detail::no_node, key_);
        if (z_order_ != 0) {
            s.set_z_order(rows_id, z_order_);
        }

        flatten_ctx ctx{s, pi, expanded_, cursor_, node_sty_, highlight_sty_};
        int flat_index = 0;
        if (!root_.label.empty()) {
            flatten_node(ctx, root_, 0, flat_index);
        } else {
            for (const auto &child : root_.children) {
                flatten_node(ctx, child, 0, flat_index);
            }
        }

        return rows_id;
    }

} // namespace tapiru
