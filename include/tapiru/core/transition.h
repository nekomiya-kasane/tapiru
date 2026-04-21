#pragma once

/**
 * @file transition.h
 * @brief Higher-level transition effects built on the tween/animation system.
 *
 * Provides typewriter, counter, marquee, and cross-fade transition helpers.
 */

#include "tapiru/core/animation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace tapiru {

// ── Typewriter effect ────────────────────────────────────────────────

class typewriter {
  public:
    typewriter() = default;

    explicit typewriter(std::string_view text, duration_ms dur = duration_ms(1000),
                        duration_ms char_delay = duration_ms(0))
        : text_(text), total_dur_(dur), char_delay_(char_delay) {}

    void start(time_point now) {
        start_ = now;
        started_ = true;
    }

    [[nodiscard]] std::string value(time_point now) const {
        if (!started_) {
            return "";
        }
        if (text_.empty()) {
            return "";
        }

        auto elapsed = std::chrono::duration_cast<duration_ms>(now - start_);
        int total_chars = static_cast<int>(text_.size());

        int visible;
        if (char_delay_.count() > 0) {
            visible = static_cast<int>(elapsed.count() / char_delay_.count());
        } else if (total_dur_.count() > 0) {
            float t = static_cast<float>(elapsed.count()) / static_cast<float>(total_dur_.count());
            if (t > 1.0f) {
                t = 1.0f;
            }
            visible = static_cast<int>(t * static_cast<float>(total_chars));
        } else {
            visible = total_chars;
        }

        visible = std::clamp(visible, 0, total_chars);
        return text_.substr(0, static_cast<size_t>(visible));
    }

    [[nodiscard]] bool finished(time_point now) const {
        if (!started_) {
            return false;
        }
        auto elapsed = std::chrono::duration_cast<duration_ms>(now - start_);
        if (char_delay_.count() > 0) {
            return elapsed.count() >= char_delay_.count() * static_cast<long long>(text_.size());
        }
        return elapsed >= total_dur_;
    }

    [[nodiscard]] bool started() const noexcept { return started_; }

  private:
    std::string text_;
    duration_ms total_dur_{1000};
    duration_ms char_delay_{0};
    time_point start_;
    bool started_ = false;
};

// ── Counter animation ────────────────────────────────────────────────

class counter_animation {
  public:
    counter_animation() = default;

    counter_animation(double from, double to, duration_ms dur, easing_fn ease = easing::ease_out)
        : tw_(static_cast<float>(from), static_cast<float>(to), dur, std::move(ease)) {}

    void start(time_point now) { tw_.start(now); }

    [[nodiscard]] double value(time_point now) const { return static_cast<double>(tw_.value(now)); }

    [[nodiscard]] int int_value(time_point now) const { return static_cast<int>(std::round(tw_.value(now))); }

    [[nodiscard]] bool finished(time_point now) const { return tw_.finished(now); }
    [[nodiscard]] bool started() const noexcept { return tw_.started(); }

  private:
    tween tw_;
};

// ── Marquee (scrolling text) ─────────────────────────────────────────

class marquee {
  public:
    marquee() = default;

    explicit marquee(std::string_view text, uint32_t visible_width, duration_ms scroll_interval = duration_ms(200))
        : text_(text), visible_w_(visible_width), interval_(scroll_interval) {}

    void start(time_point now) {
        start_ = now;
        started_ = true;
    }

    [[nodiscard]] std::string value(time_point now) const {
        if (!started_ || text_.empty() || visible_w_ == 0) {
            return "";
        }

        if (text_.size() <= visible_w_) {
            return std::string(text_);
        }

        auto elapsed = std::chrono::duration_cast<duration_ms>(now - start_);
        int total_len = static_cast<int>(text_.size()) + static_cast<int>(visible_w_);
        int offset = 0;
        if (interval_.count() > 0) {
            offset = static_cast<int>(elapsed.count() / interval_.count()) % total_len;
        }

        // Build visible window with wrapping
        std::string padded = text_ + std::string(visible_w_, ' ') + text_;
        return padded.substr(static_cast<size_t>(offset), visible_w_);
    }

    [[nodiscard]] bool started() const noexcept { return started_; }

  private:
    std::string text_;
    uint32_t visible_w_ = 0;
    duration_ms interval_{200};
    time_point start_;
    bool started_ = false;
};

// ── Cross-fade transition ────────────────────────────────────────────

class cross_fade {
  public:
    cross_fade() = default;

    explicit cross_fade(duration_ms dur, easing_fn ease = easing::ease_in_out)
        : tw_(0.0f, 1.0f, dur, std::move(ease)) {}

    void start(time_point now) { tw_.start(now); }

    [[nodiscard]] float progress(time_point now) const { return tw_.value(now); }

    [[nodiscard]] uint8_t old_alpha(time_point now) const {
        float p = progress(now);
        return static_cast<uint8_t>((1.0f - p) * 255.0f);
    }

    [[nodiscard]] uint8_t new_alpha(time_point now) const {
        float p = progress(now);
        return static_cast<uint8_t>(p * 255.0f);
    }

    [[nodiscard]] bool finished(time_point now) const { return tw_.finished(now); }
    [[nodiscard]] bool started() const noexcept { return tw_.started(); }

  private:
    tween tw_;
};

// ── Slide transition ─────────────────────────────────────────────────

enum class slide_direction : uint8_t { left, right, up, down };

class slide_transition {
  public:
    slide_transition() = default;

    slide_transition(slide_direction dir, float distance, duration_ms dur, easing_fn ease = easing::ease_out)
        : dir_(dir), tw_(distance, 0.0f, dur, std::move(ease)) {}

    void start(time_point now) { tw_.start(now); }

    [[nodiscard]] int offset_x(time_point now) const {
        float v = tw_.value(now);
        switch (dir_) {
        case slide_direction::left:
            return -static_cast<int>(v);
        case slide_direction::right:
            return static_cast<int>(v);
        default:
            return 0;
        }
    }

    [[nodiscard]] int offset_y(time_point now) const {
        float v = tw_.value(now);
        switch (dir_) {
        case slide_direction::up:
            return -static_cast<int>(v);
        case slide_direction::down:
            return static_cast<int>(v);
        default:
            return 0;
        }
    }

    [[nodiscard]] bool finished(time_point now) const { return tw_.finished(now); }
    [[nodiscard]] bool started() const noexcept { return tw_.started(); }

  private:
    slide_direction dir_ = slide_direction::left;
    tween tw_;
};

} // namespace tapiru
