#pragma once

/**
 * @file spinner.h
 * @brief Animated spinner widget for live displays.
 *
 * Usage:
 *   auto sp = std::make_shared<tapiru::spinner_state>();
 *   live lv(con);
 *   lv.set(spinner_builder(sp).message("Loading..."));
 *   // spinner auto-advances each render frame
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/text/markup.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

namespace detail {
class scene;
}
using node_id = uint32_t;

// ── Predefined spinner frame sets ───────────────────────────────────────

struct spinner_frames {
    static const std::vector<std::string> &dots() {
        static const std::vector<std::string> f = {"\xe2\xa0\x8b", "\xe2\xa0\x99", "\xe2\xa0\xb9", "\xe2\xa0\xb8",
                                                   "\xe2\xa0\xbc", "\xe2\xa0\xb4", "\xe2\xa0\xa6", "\xe2\xa0\xa7",
                                                   "\xe2\xa0\x87", "\xe2\xa0\x8f"}; // ⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏
        return f;
    }
    static const std::vector<std::string> &line() {
        static const std::vector<std::string> f = {"-", "\\", "|", "/"};
        return f;
    }
    static const std::vector<std::string> &arc() {
        static const std::vector<std::string> f = {"\xe2\x97\x9c", "\xe2\x97\xa0", "\xe2\x97\x9d",
                                                   "\xe2\x97\x9f"}; // ◜◠◝◟
        return f;
    }
};

// ── Spinner state (thread-safe) ─────────────────────────────────────────

class TAPIRU_API spinner_state {
  public:
    spinner_state() = default;

    /** @brief Advance to next frame. Thread-safe. */
    void tick() noexcept { frame_.fetch_add(1, std::memory_order_relaxed); }

    /** @brief Get current frame index. Thread-safe. */
    [[nodiscard]] uint32_t frame() const noexcept { return frame_.load(std::memory_order_relaxed); }

    /** @brief Mark spinner as done. Thread-safe. */
    void set_done() noexcept { done_.store(true, std::memory_order_release); }

    [[nodiscard]] bool done() const noexcept { return done_.load(std::memory_order_acquire); }

  private:
    std::atomic<uint32_t> frame_{0};
    std::atomic<bool> done_{false};
};

// ── Spinner builder ─────────────────────────────────────────────────────

class TAPIRU_API spinner_builder {
  public:
    explicit spinner_builder(std::shared_ptr<spinner_state> state)
        : state_(std::move(state)), frames_(spinner_frames::dots()) {}

    spinner_builder &message(std::string_view msg) {
        message_ = msg;
        return *this;
    }
    spinner_builder &frames(std::vector<std::string> f) {
        frames_ = std::move(f);
        return *this;
    }
    spinner_builder &spinner_style(const style &s) {
        spinner_sty_ = s;
        return *this;
    }
    spinner_builder &done_text(std::string_view t) {
        done_text_ = t;
        return *this;
    }
    spinner_builder &key(std::string_view k);
    spinner_builder &z_order(int16_t z) {
        z_order_ = z;
        return *this;
    }

    node_id flatten_into(detail::scene &s) const;

  private:
    std::shared_ptr<spinner_state> state_;
    std::vector<std::string> frames_;
    std::string message_;
    style spinner_sty_;
    std::string done_text_ = "\xe2\x9c\x93"; // ✓
    uint64_t key_ = 0;
    int16_t z_order_ = 0;
};

} // namespace tapiru
