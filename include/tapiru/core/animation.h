#pragma once

/**
 * @file animation.h
 * @brief Declarative animation system: tween, easing, animated<T>.
 */

#include "tapiru/exports.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>

namespace tapiru {

// ── Time alias ─────────────────────────────────────────────────────────

using steady_clock = std::chrono::steady_clock;
using time_point = steady_clock::time_point;
using duration_ms = std::chrono::milliseconds;

// ── Easing functions ───────────────────────────────────────────────────

/** @brief Easing function signature: t ∈ [0,1] → [0,1]. */
using easing_fn = std::function<float(float)>;

namespace easing {

inline float linear(float t) {
    return t;
}

inline float ease_in(float t) {
    return t * t;
}

inline float ease_out(float t) {
    return t * (2.0f - t);
}

inline float ease_in_out(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline float bounce(float t) {
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

inline float elastic(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.075f) * (2.0f * 3.14159265f) / 0.3f) + 1.0f;
}

} // namespace easing

// ── Tween ──────────────────────────────────────────────────────────────

/**
 * @brief A single tween animation from one float value to another.
 */
class tween {
  public:
    tween() = default;

    tween(float from, float to, duration_ms dur, easing_fn ease = easing::linear)
        : from_(from), to_(to), duration_(dur), ease_(std::move(ease)) {}

    /** @brief Start or restart the tween at the given time. */
    void start(time_point now) {
        start_time_ = now;
        started_ = true;
    }

    /** @brief Get the current interpolated value. */
    [[nodiscard]] float value(time_point now) const {
        if (!started_) return from_;
        auto elapsed = std::chrono::duration_cast<duration_ms>(now - start_time_);
        if (elapsed >= duration_) return to_;
        float t = static_cast<float>(elapsed.count()) / static_cast<float>(duration_.count());
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        float eased = ease_ ? ease_(t) : t;
        return from_ + (to_ - from_) * eased;
    }

    /** @brief Check if the tween has finished. */
    [[nodiscard]] bool finished(time_point now) const {
        if (!started_) return false;
        return std::chrono::duration_cast<duration_ms>(now - start_time_) >= duration_;
    }

    [[nodiscard]] bool started() const noexcept { return started_; }
    [[nodiscard]] float from() const noexcept { return from_; }
    [[nodiscard]] float to() const noexcept { return to_; }
    [[nodiscard]] duration_ms duration() const noexcept { return duration_; }

  private:
    float from_ = 0.0f;
    float to_ = 0.0f;
    duration_ms duration_ = duration_ms(0);
    easing_fn ease_ = easing::linear;
    time_point start_time_;
    bool started_ = false;
};

// ── animated<T> ────────────────────────────────────────────────────────

/**
 * @brief An animated value that smoothly transitions between targets.
 *
 * Works for any type T that supports: T + (T - T) * float
 * Specializations provided for float, uint8_t (alpha), int.
 */
template <typename T> class animated {
  public:
    explicit animated(T initial = T{}) : current_(initial), target_(initial) {}

    /** @brief Get the current animated value. */
    [[nodiscard]] T value(time_point now) const {
        if (!tw_.started()) return current_;
        float v = tw_.value(now);
        if (tw_.finished(now)) return target_;
        return static_cast<T>(static_cast<float>(current_) +
                              (static_cast<float>(target_) - static_cast<float>(current_)) * (v - tw_.from()) /
                                  (tw_.to() - tw_.from()));
    }

    /** @brief Start animating to a new target. */
    void animate_to(T target, duration_ms dur, easing_fn ease, time_point now) {
        current_ = value(now);
        target_ = target;
        tw_ = tween(0.0f, 1.0f, dur, std::move(ease));
        tw_.start(now);
    }

    /** @brief Set value immediately without animation. */
    void set(T val) {
        current_ = val;
        target_ = val;
        tw_ = tween();
    }

    [[nodiscard]] T target() const noexcept { return target_; }
    [[nodiscard]] bool animating(time_point now) const { return tw_.started() && !tw_.finished(now); }

  private:
    T current_;
    T target_;
    tween tw_;
};

// ── Preset animations ──────────────────────────────────────────────────

/** @brief Create a fade-in tween (alpha 0→255). */
inline tween fade_in(duration_ms dur = duration_ms(300), easing_fn ease = easing::ease_out) {
    return tween(0.0f, 255.0f, dur, std::move(ease));
}

/** @brief Create a fade-out tween (alpha 255→0). */
inline tween fade_out(duration_ms dur = duration_ms(300), easing_fn ease = easing::ease_in) {
    return tween(255.0f, 0.0f, dur, std::move(ease));
}

/** @brief Create a slide-in tween (offset width→0). */
inline tween slide_in(float from_offset, duration_ms dur = duration_ms(400), easing_fn ease = easing::ease_out) {
    return tween(from_offset, 0.0f, dur, std::move(ease));
}

} // namespace tapiru
