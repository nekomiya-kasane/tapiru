/**
 * @file progress.cpp
 * @brief Progress builder flatten_into implementation.
 */

#include "tapiru/widgets/progress.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

#include <cstdio>
#include <string>

namespace tapiru {

    progress_builder &progress_builder::key(std::string_view k) {
        key_ = detail::fnv1a_hash(k);
        return *this;
    }

    node_id progress_builder::flatten_into(detail::scene &s) const {
        // Each task becomes a text node showing: "description  [████░░░░] 50%"
        // All tasks are stacked vertically via a columns_data with 1 column,
        // but simpler: we create a single text node with newlines for multi-task.
        // Actually, we create one text node per task and stack them in a padding node.
        // Simplest: create a single text node with all tasks concatenated.

        if (tasks_.empty()) {
            detail::text_data td;
            td.fragments.push_back({"(no tasks)", style{}});
            auto pi = s.add_text(std::move(td));
            return s.add_node(detail::widget_type::text, pi);
        }

        if (tasks_.size() == 1) {
            // Single task — just one text line
            return build_task_node(s, *tasks_[0]);
        }

        // Multiple tasks — stack vertically using a columns layout won't work
        // (columns is horizontal). Instead, create a padding wrapper with
        // children linked as siblings. We'll use a simple approach:
        // create a "columns" node with 1 child per task, but that's horizontal.
        //
        // Better: create multiple text nodes as siblings under a dummy padding node.
        // The padding widget only renders its single content child though.
        //
        // Simplest correct approach: build a single text node with all lines.
        detail::text_data td;
        for (size_t i = 0; i < tasks_.size(); ++i) {
            if (i > 0) {
                td.fragments.push_back({"\n", style{}});
            }
            build_task_fragments(td.fragments, *tasks_[i]);
        }
        td.overflow = overflow_mode::truncate;
        auto pi = s.add_text(std::move(td));
        return s.add_node(detail::widget_type::text, pi);
    }

    node_id progress_builder::build_task_node(detail::scene &s, const progress_task &task) const {
        detail::text_data td;
        build_task_fragments(td.fragments, task);
        td.overflow = overflow_mode::truncate;
        auto pi = s.add_text(std::move(td));
        return s.add_node(detail::widget_type::text, pi);
    }

    void progress_builder::build_task_fragments(std::vector<text_fragment> &frags, const progress_task &task) const {
        // Description
        frags.push_back({task.description(), style{}});
        frags.push_back({"  ", style{}});

        // Bar
        double frac = task.fraction();
        uint32_t filled = static_cast<uint32_t>(frac * bar_width_);
        if (filled > bar_width_) {
            filled = bar_width_;
        }

        std::string bar_str;
        bar_str.reserve(bar_width_ * 4);
        for (uint32_t i = 0; i < filled; ++i) {
            append_char32(bar_str, complete_ch_);
        }
        for (uint32_t i = filled; i < bar_width_; ++i) {
            append_char32(bar_str, remaining_ch_);
        }
        frags.push_back({bar_str, bar_sty_});

        // Percentage
        char pct_buf[16];
        std::snprintf(pct_buf, sizeof(pct_buf), " %3d%%", static_cast<int>(frac * 100.0));
        frags.push_back({pct_buf, style{}});

        // Completed indicator
        if (task.completed()) {
            frags.push_back({" \xe2\x9c\x93", style{}}); // ✓ in UTF-8
        }
    }

    void progress_builder::append_char32(std::string &out, char32_t cp) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

} // namespace tapiru
