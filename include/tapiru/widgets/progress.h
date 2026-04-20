#pragma once

/**
 * @file progress.h
 * @brief Thread-safe progress bar state + builder.
 *
 * Usage with live display:
 *   auto task = std::make_shared<tapiru::progress_task>("Downloading", 100);
 *   live lv(con);
 *   lv.set(progress_builder().add_task(task));
 *   // From worker thread:
 *   task->advance(10);
 */

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"
#include "tapiru/text/markup.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

// ── Progress task (thread-safe state) ───────────────────────────────────

/**
 * @brief A single progress task with atomic state.
 *
 * Shared between the user thread (which calls advance/set_completed)
 * and the render thread (which reads current/total for display).
 */
class TAPIRU_API progress_task {
public:
    progress_task(std::string description, uint64_t total)
        : description_(std::move(description))
        , total_(total)
        , current_(0)
        , completed_(false) {}

    /** @brief Advance progress by delta. Thread-safe. */
    void advance(uint64_t delta) noexcept {
        current_.fetch_add(delta, std::memory_order_relaxed);
    }

    /** @brief Set absolute progress value. Thread-safe. */
    void set_current(uint64_t value) noexcept {
        current_.store(value, std::memory_order_relaxed);
    }

    /** @brief Mark task as completed. Thread-safe. */
    void set_completed() noexcept {
        current_.store(total_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        completed_.store(true, std::memory_order_release);
    }

    /** @brief Update total. Thread-safe. */
    void set_total(uint64_t total) noexcept {
        total_.store(total, std::memory_order_relaxed);
    }

    [[nodiscard]] const std::string& description() const noexcept { return description_; }
    [[nodiscard]] uint64_t current() const noexcept { return current_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t total() const noexcept { return total_.load(std::memory_order_relaxed); }
    [[nodiscard]] bool completed() const noexcept { return completed_.load(std::memory_order_acquire); }

    /** @brief Get progress as a fraction [0.0, 1.0]. */
    [[nodiscard]] double fraction() const noexcept {
        auto t = total();
        if (t == 0) return 0.0;
        auto c = current();
        return static_cast<double>(c > t ? t : c) / static_cast<double>(t);
    }

private:
    std::string        description_;
    std::atomic<uint64_t> total_;
    std::atomic<uint64_t> current_;
    std::atomic<bool>     completed_;
};

// ── Progress builder ────────────────────────────────────────────────────

class TAPIRU_API progress_builder {
public:
    progress_builder() = default;

    progress_builder& add_task(std::shared_ptr<progress_task> task) {
        tasks_.push_back(std::move(task));
        return *this;
    }

    progress_builder& bar_width(uint32_t w)       { bar_width_ = w; return *this; }
    progress_builder& bar_style(const style& s)    { bar_sty_ = s; return *this; }
    progress_builder& complete_char(char32_t ch)   { complete_ch_ = ch; return *this; }
    progress_builder& remaining_char(char32_t ch)  { remaining_ch_ = ch; return *this; }
    progress_builder& key(std::string_view k);
    progress_builder& z_order(int16_t z) { z_order_ = z; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    node_id build_task_node(detail::scene& s, const progress_task& task) const;
    void build_task_fragments(std::vector<text_fragment>& frags, const progress_task& task) const;
    static void append_char32(std::string& out, char32_t cp);

    std::vector<std::shared_ptr<progress_task>> tasks_;
    uint32_t bar_width_    = 40;
    style    bar_sty_;
    char32_t complete_ch_  = U'\x2588';  // █
    char32_t remaining_ch_ = U'\x2591';  // ░
    uint64_t key_          = 0;
    int16_t  z_order_      = 0;
};

}  // namespace tapiru
