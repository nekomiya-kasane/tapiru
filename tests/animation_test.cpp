/**
 * @file animation_test.cpp
 * @brief Tests for tween, easing functions, and animated<T>.
 */

#include <gtest/gtest.h>
#include "tapiru/core/animation.h"

using namespace tapiru;

// ── Easing functions ───────────────────────────────────────────────────

TEST(EasingTest, LinearIdentity) {
    EXPECT_FLOAT_EQ(easing::linear(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(easing::linear(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(easing::linear(1.0f), 1.0f);
}

TEST(EasingTest, EaseInStartsSlowEndsAtOne) {
    EXPECT_FLOAT_EQ(easing::ease_in(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(easing::ease_in(1.0f), 1.0f);
    EXPECT_LT(easing::ease_in(0.5f), 0.5f);  // slower than linear at midpoint
}

TEST(EasingTest, EaseOutStartsFastEndsAtOne) {
    EXPECT_FLOAT_EQ(easing::ease_out(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(easing::ease_out(1.0f), 1.0f);
    EXPECT_GT(easing::ease_out(0.5f), 0.5f);  // faster than linear at midpoint
}

TEST(EasingTest, EaseInOutEndpoints) {
    EXPECT_FLOAT_EQ(easing::ease_in_out(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(easing::ease_in_out(1.0f), 1.0f);
}

TEST(EasingTest, BounceEndpoints) {
    EXPECT_NEAR(easing::bounce(0.0f), 0.0f, 0.001f);
    EXPECT_NEAR(easing::bounce(1.0f), 1.0f, 0.001f);
}

TEST(EasingTest, ElasticEndpoints) {
    EXPECT_FLOAT_EQ(easing::elastic(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(easing::elastic(1.0f), 1.0f);
}

// ── Tween ──────────────────────────────────────────────────────────────

TEST(TweenTest, DefaultTween) {
    tween tw;
    auto now = steady_clock::now();
    EXPECT_FLOAT_EQ(tw.value(now), 0.0f);
    EXPECT_FALSE(tw.started());
    EXPECT_FALSE(tw.finished(now));
}

TEST(TweenTest, LinearTweenMidpoint) {
    tween tw(0.0f, 100.0f, duration_ms(1000), easing::linear);
    auto start = steady_clock::now();
    tw.start(start);

    // At start
    EXPECT_FLOAT_EQ(tw.value(start), 0.0f);

    // At midpoint (simulate 500ms later)
    auto mid = start + duration_ms(500);
    EXPECT_NEAR(tw.value(mid), 50.0f, 1.0f);

    // At end
    auto end = start + duration_ms(1000);
    EXPECT_FLOAT_EQ(tw.value(end), 100.0f);
    EXPECT_TRUE(tw.finished(end));
}

TEST(TweenTest, TweenBeforeStart) {
    tween tw(10.0f, 90.0f, duration_ms(500));
    auto now = steady_clock::now();
    // Not started yet — should return from value
    EXPECT_FLOAT_EQ(tw.value(now), 10.0f);
}

TEST(TweenTest, TweenAfterEnd) {
    tween tw(0.0f, 100.0f, duration_ms(100));
    auto start = steady_clock::now();
    tw.start(start);
    auto way_after = start + duration_ms(5000);
    EXPECT_FLOAT_EQ(tw.value(way_after), 100.0f);
    EXPECT_TRUE(tw.finished(way_after));
}

TEST(TweenTest, TweenWithEaseIn) {
    tween tw(0.0f, 100.0f, duration_ms(1000), easing::ease_in);
    auto start = steady_clock::now();
    tw.start(start);
    auto mid = start + duration_ms(500);
    // ease_in at 0.5 = 0.25, so value should be ~25
    EXPECT_NEAR(tw.value(mid), 25.0f, 2.0f);
}

TEST(TweenTest, Accessors) {
    tween tw(10.0f, 90.0f, duration_ms(500));
    EXPECT_FLOAT_EQ(tw.from(), 10.0f);
    EXPECT_FLOAT_EQ(tw.to(), 90.0f);
    EXPECT_EQ(tw.duration(), duration_ms(500));
}

// ── animated<T> ────────────────────────────────────────────────────────

TEST(AnimatedTest, InitialValue) {
    animated<float> a(42.0f);
    auto now = steady_clock::now();
    EXPECT_FLOAT_EQ(a.value(now), 42.0f);
    EXPECT_FALSE(a.animating(now));
}

TEST(AnimatedTest, AnimateToTarget) {
    animated<float> a(0.0f);
    auto start = steady_clock::now();
    a.animate_to(100.0f, duration_ms(1000), easing::linear, start);

    EXPECT_TRUE(a.animating(start));
    EXPECT_FLOAT_EQ(a.target(), 100.0f);

    // Midpoint
    auto mid = start + duration_ms(500);
    EXPECT_NEAR(a.value(mid), 50.0f, 2.0f);

    // After completion
    auto end = start + duration_ms(1500);
    EXPECT_FLOAT_EQ(a.value(end), 100.0f);
    EXPECT_FALSE(a.animating(end));
}

TEST(AnimatedTest, SetImmediate) {
    animated<float> a(0.0f);
    a.set(99.0f);
    auto now = steady_clock::now();
    EXPECT_FLOAT_EQ(a.value(now), 99.0f);
    EXPECT_FALSE(a.animating(now));
}

TEST(AnimatedTest, AnimateInt) {
    animated<int> a(0);
    auto start = steady_clock::now();
    a.animate_to(100, duration_ms(1000), easing::linear, start);
    auto mid = start + duration_ms(500);
    int v = a.value(mid);
    EXPECT_GT(v, 30);
    EXPECT_LT(v, 70);
}

// ── Presets ────────────────────────────────────────────────────────────

TEST(AnimationPresetTest, FadeIn) {
    auto tw = fade_in(duration_ms(300));
    EXPECT_FLOAT_EQ(tw.from(), 0.0f);
    EXPECT_FLOAT_EQ(tw.to(), 255.0f);
    EXPECT_EQ(tw.duration(), duration_ms(300));
}

TEST(AnimationPresetTest, FadeOut) {
    auto tw = fade_out(duration_ms(300));
    EXPECT_FLOAT_EQ(tw.from(), 255.0f);
    EXPECT_FLOAT_EQ(tw.to(), 0.0f);
}

TEST(AnimationPresetTest, SlideIn) {
    auto tw = slide_in(80.0f, duration_ms(400));
    EXPECT_FLOAT_EQ(tw.from(), 80.0f);
    EXPECT_FLOAT_EQ(tw.to(), 0.0f);
}
