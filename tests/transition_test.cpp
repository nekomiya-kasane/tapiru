/**
 * @file transition_test.cpp
 * @brief Tests for typewriter, counter_animation, marquee, cross_fade, slide_transition.
 */

#include <gtest/gtest.h>
#include "tapiru/core/transition.h"

using namespace tapiru;

// Helper: make a time_point offset from epoch
static time_point tp(long long ms) {
    return time_point(duration_ms(ms));
}

// ══════════════════════════════════════════════════════════════════════════
//  Typewriter
// ══════════════════════════════════════════════════════════════════════════

TEST(TypewriterTest, NotStartedReturnsEmpty) {
    typewriter tw("Hello", duration_ms(1000));
    EXPECT_EQ(tw.value(tp(500)), "");
    EXPECT_FALSE(tw.started());
    EXPECT_FALSE(tw.finished(tp(500)));
}

TEST(TypewriterTest, EmptyTextAlwaysEmpty) {
    typewriter tw("", duration_ms(1000));
    tw.start(tp(0));
    EXPECT_EQ(tw.value(tp(500)), "");
}

TEST(TypewriterTest, FullTextAfterDuration) {
    typewriter tw("Hello", duration_ms(1000));
    tw.start(tp(0));
    EXPECT_EQ(tw.value(tp(2000)), "Hello");
    EXPECT_TRUE(tw.finished(tp(2000)));
}

TEST(TypewriterTest, PartialTextMidway) {
    typewriter tw("ABCDE", duration_ms(1000));
    tw.start(tp(0));
    // At t=500ms, 50% through, should show ~2-3 chars
    auto v = tw.value(tp(500));
    EXPECT_GE(v.size(), 1u);
    EXPECT_LE(v.size(), 4u);
}

TEST(TypewriterTest, CharDelayMode) {
    // 5 chars, 100ms per char = 500ms total
    typewriter tw("ABCDE", duration_ms(0), duration_ms(100));
    tw.start(tp(0));
    EXPECT_EQ(tw.value(tp(0)), "");        // 0 chars at t=0
    EXPECT_EQ(tw.value(tp(100)), "A");     // 1 char at t=100
    EXPECT_EQ(tw.value(tp(250)), "AB");    // 2 chars at t=250
    EXPECT_EQ(tw.value(tp(500)), "ABCDE"); // all at t=500
    EXPECT_TRUE(tw.finished(tp(500)));
    EXPECT_FALSE(tw.finished(tp(400)));
}

TEST(TypewriterTest, ZeroDurationShowsAll) {
    typewriter tw("Hello", duration_ms(0));
    tw.start(tp(0));
    EXPECT_EQ(tw.value(tp(0)), "Hello");
}

// ══════════════════════════════════════════════════════════════════════════
//  Counter animation
// ══════════════════════════════════════════════════════════════════════════

TEST(CounterAnimationTest, NotStarted) {
    counter_animation ca(0, 100, duration_ms(1000));
    EXPECT_FALSE(ca.started());
}

TEST(CounterAnimationTest, ReachesTarget) {
    counter_animation ca(0, 100, duration_ms(1000), easing::linear);
    ca.start(tp(0));
    EXPECT_TRUE(ca.started());

    // After full duration, should be at target
    EXPECT_NEAR(ca.value(tp(2000)), 100.0, 1.0);
    EXPECT_TRUE(ca.finished(tp(2000)));
}

TEST(CounterAnimationTest, IntValueRounds) {
    counter_animation ca(0, 10, duration_ms(1000), easing::linear);
    ca.start(tp(0));
    // At midpoint with linear easing, should be ~5
    int mid = ca.int_value(tp(500));
    EXPECT_GE(mid, 4);
    EXPECT_LE(mid, 6);
}

// ══════════════════════════════════════════════════════════════════════════
//  Marquee
// ══════════════════════════════════════════════════════════════════════════

TEST(MarqueeTest, NotStartedReturnsEmpty) {
    marquee m("Hello World", 5, duration_ms(100));
    EXPECT_EQ(m.value(tp(0)), "");
    EXPECT_FALSE(m.started());
}

TEST(MarqueeTest, ShortTextReturnsFullText) {
    marquee m("Hi", 10, duration_ms(100));
    m.start(tp(0));
    EXPECT_TRUE(m.started());
    EXPECT_EQ(m.value(tp(0)), "Hi");
}

TEST(MarqueeTest, EmptyTextReturnsEmpty) {
    marquee m("", 5, duration_ms(100));
    m.start(tp(0));
    EXPECT_EQ(m.value(tp(100)), "");
}

TEST(MarqueeTest, ZeroWidthReturnsEmpty) {
    marquee m("Hello", 0, duration_ms(100));
    m.start(tp(0));
    EXPECT_EQ(m.value(tp(100)), "");
}

TEST(MarqueeTest, ScrollsOverTime) {
    marquee m("ABCDEFGHIJ", 5, duration_ms(100));
    m.start(tp(0));
    auto v0 = m.value(tp(0));
    auto v1 = m.value(tp(100));
    EXPECT_EQ(v0.size(), 5u);
    EXPECT_EQ(v1.size(), 5u);
    // Different scroll positions should yield different strings (usually)
    // At t=0 offset=0, at t=100 offset=1
    EXPECT_NE(v0, v1);
}

// ══════════════════════════════════════════════════════════════════════════
//  Cross-fade
// ══════════════════════════════════════════════════════════════════════════

TEST(CrossFadeTest, NotStarted) {
    cross_fade cf(duration_ms(1000));
    EXPECT_FALSE(cf.started());
}

TEST(CrossFadeTest, StartsAtZero) {
    cross_fade cf(duration_ms(1000), easing::linear);
    cf.start(tp(0));
    EXPECT_TRUE(cf.started());
    float p = cf.progress(tp(0));
    EXPECT_NEAR(p, 0.0f, 0.05f);
}

TEST(CrossFadeTest, EndsAtOne) {
    cross_fade cf(duration_ms(1000), easing::linear);
    cf.start(tp(0));
    EXPECT_NEAR(cf.progress(tp(2000)), 1.0f, 0.05f);
    EXPECT_TRUE(cf.finished(tp(2000)));
}

TEST(CrossFadeTest, AlphaValues) {
    cross_fade cf(duration_ms(1000), easing::linear);
    cf.start(tp(0));

    // At start: old visible, new invisible
    EXPECT_GE(cf.old_alpha(tp(0)), 240);
    EXPECT_LE(cf.new_alpha(tp(0)), 15);

    // At end: new visible, old invisible
    EXPECT_LE(cf.old_alpha(tp(2000)), 15);
    EXPECT_GE(cf.new_alpha(tp(2000)), 240);
}

// ══════════════════════════════════════════════════════════════════════════
//  Slide transition
// ══════════════════════════════════════════════════════════════════════════

TEST(SlideTransitionTest, NotStarted) {
    slide_transition st(slide_direction::left, 100.0f, duration_ms(500));
    EXPECT_FALSE(st.started());
}

TEST(SlideTransitionTest, LeftSlideOffsetX) {
    slide_transition st(slide_direction::left, 100.0f, duration_ms(500), easing::linear);
    st.start(tp(0));
    EXPECT_TRUE(st.started());

    // At start, offset_x should be large negative
    EXPECT_LT(st.offset_x(tp(0)), 0);
    // offset_y should be 0 for horizontal
    EXPECT_EQ(st.offset_y(tp(0)), 0);

    // After completion, offset should be 0
    EXPECT_EQ(st.offset_x(tp(1000)), 0);
    EXPECT_TRUE(st.finished(tp(1000)));
}

TEST(SlideTransitionTest, RightSlideOffsetX) {
    slide_transition st(slide_direction::right, 100.0f, duration_ms(500), easing::linear);
    st.start(tp(0));
    EXPECT_GT(st.offset_x(tp(0)), 0);
    EXPECT_EQ(st.offset_y(tp(0)), 0);
    EXPECT_EQ(st.offset_x(tp(1000)), 0);
}

TEST(SlideTransitionTest, UpSlideOffsetY) {
    slide_transition st(slide_direction::up, 50.0f, duration_ms(300), easing::linear);
    st.start(tp(0));
    EXPECT_LT(st.offset_y(tp(0)), 0);
    EXPECT_EQ(st.offset_x(tp(0)), 0);
}

TEST(SlideTransitionTest, DownSlideOffsetY) {
    slide_transition st(slide_direction::down, 50.0f, duration_ms(300), easing::linear);
    st.start(tp(0));
    EXPECT_GT(st.offset_y(tp(0)), 0);
    EXPECT_EQ(st.offset_x(tp(0)), 0);
}
