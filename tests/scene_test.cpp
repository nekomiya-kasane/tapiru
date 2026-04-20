#include <gtest/gtest.h>

// Internal headers — tests are allowed to include these
#include "detail/scene.h"
#include "detail/widget_registry.h"

using namespace tapiru;
using namespace tapiru::detail;

// ── Scene topology ──────────────────────────────────────────────────────

TEST(SceneTest, AddNode) {
    scene s;
    auto td = text_data{{{{"Hello", style{}}}}, overflow_mode::wrap, justify::left};
    auto pi = s.add_text(std::move(td));
    auto id = s.add_node(widget_type::text, pi);

    EXPECT_EQ(s.node_count(), 1u);
    EXPECT_EQ(s.type_of(id), widget_type::text);
    EXPECT_EQ(s.pool_of(id), pi);
    EXPECT_EQ(s.parent_of(id), no_node);
    EXPECT_EQ(s.first_child(id), no_node);
}

TEST(SceneTest, ParentChildLink) {
    scene s;
    auto pi_panel = s.add_panel(panel_data{});
    auto panel_id = s.add_node(widget_type::panel, pi_panel);

    auto pi_text = s.add_text(text_data{{{{"Child", style{}}}}, overflow_mode::wrap, justify::left});
    auto text_id = s.add_node(widget_type::text, pi_text, panel_id);

    EXPECT_EQ(s.first_child(panel_id), text_id);
    EXPECT_EQ(s.parent_of(text_id), panel_id);
    EXPECT_EQ(s.next_sibling(text_id), no_node);
}

TEST(SceneTest, MultipleSiblings) {
    scene s;
    auto pi_cols = s.add_columns(columns_data{});
    auto cols_id = s.add_node(widget_type::columns, pi_cols);

    auto pi1 = s.add_text(text_data{{{{"A", style{}}}}, overflow_mode::wrap, justify::left});
    auto id1 = s.add_node(widget_type::text, pi1, cols_id);

    auto pi2 = s.add_text(text_data{{{{"B", style{}}}}, overflow_mode::wrap, justify::left});
    auto id2 = s.add_node(widget_type::text, pi2, cols_id);

    auto pi3 = s.add_text(text_data{{{{"C", style{}}}}, overflow_mode::wrap, justify::left});
    auto id3 = s.add_node(widget_type::text, pi3, cols_id);

    EXPECT_EQ(s.first_child(cols_id), id1);
    EXPECT_EQ(s.next_sibling(id1), id2);
    EXPECT_EQ(s.next_sibling(id2), id3);
    EXPECT_EQ(s.next_sibling(id3), no_node);
}

// ── Text measure/render ─────────────────────────────────────────────────

TEST(SceneTest, MeasureText) {
    scene s;
    auto pi = s.add_text(text_data{{{{"Hello", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);

    auto m = dispatch_measure(s, id, box_constraints::loose(80, 24));
    EXPECT_EQ(m.width, 5u);
    EXPECT_EQ(m.height, 1u);
}

TEST(SceneTest, RenderTextToCanvas) {
    scene s;
    auto pi = s.add_text(text_data{{{{"Hi", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);

    canvas cv(10, 1);
    dispatch_render(s, id, cv, {0, 0, 10, 1}, s.styles());

    // Check that 'H' and 'i' are in the canvas
    EXPECT_EQ(cv.get(0, 0).codepoint, U'H');
    EXPECT_EQ(cv.get(1, 0).codepoint, U'i');
}

// ── Rule measure/render ─────────────────────────────────────────────────

TEST(SceneTest, MeasureRule) {
    scene s;
    auto pi = s.add_rule(rule_data{"Title", style{}, justify::center, U'\x2500'});
    auto id = s.add_node(widget_type::rule, pi);

    auto m = dispatch_measure(s, id, box_constraints::loose(40, 24));
    EXPECT_EQ(m.width, 40u);
    EXPECT_EQ(m.height, 1u);
}

TEST(SceneTest, RenderRule) {
    scene s;
    auto pi = s.add_rule(rule_data{"", style{}, justify::center, U'-'});
    auto id = s.add_node(widget_type::rule, pi);

    canvas cv(10, 1);
    dispatch_render(s, id, cv, {0, 0, 10, 1}, s.styles());

    // All cells should be '-'
    for (uint32_t x = 0; x < 10; ++x) {
        EXPECT_EQ(cv.get(x, 0).codepoint, U'-');
    }
}

// ── Padding measure/render ──────────────────────────────────────────────

TEST(SceneTest, MeasurePadding) {
    scene s;
    auto pi_text = s.add_text(text_data{{{{"X", style{}}}}, overflow_mode::wrap, justify::left});
    auto pi_pad = s.add_padding(padding_data{1, 2, 1, 2, no_node});
    auto pad_id = s.add_node(widget_type::padding, pi_pad);
    auto text_id = s.add_node(widget_type::text, pi_text, pad_id);

    // Fix up content link
    s.get_padding(pi_pad).content = text_id;

    auto m = dispatch_measure(s, pad_id, box_constraints::loose(80, 24));
    EXPECT_EQ(m.width, 1 + 2 + 2);   // 1 char + 2 left + 2 right = 5
    EXPECT_EQ(m.height, 1 + 1 + 1);   // 1 line + 1 top + 1 bottom = 3
}

// ── Panel measure ───────────────────────────────────────────────────────

TEST(SceneTest, MeasurePanel) {
    scene s;
    auto pi_text = s.add_text(text_data{{{{"Hello", style{}}}}, overflow_mode::wrap, justify::left});
    auto pi_panel = s.add_panel(panel_data{no_node, border_style::rounded, style{}, "", style{}});
    auto panel_id = s.add_node(widget_type::panel, pi_panel);
    auto text_id = s.add_node(widget_type::text, pi_text, panel_id);

    s.get_panel(pi_panel).content = text_id;

    auto m = dispatch_measure(s, panel_id, box_constraints::loose(80, 24));
    EXPECT_EQ(m.width, 5 + 2);   // content + border
    EXPECT_EQ(m.height, 1 + 2);  // content + border
}

// ── Z-order & flags ────────────────────────────────────────────────────

TEST(SceneTest, DefaultZOrderIsZero) {
    scene s;
    auto pi = s.add_text(text_data{{{{"X", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);
    EXPECT_EQ(s.z_order(id), 0);
}

TEST(SceneTest, SetAndGetZOrder) {
    scene s;
    auto pi = s.add_text(text_data{{{{"X", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);
    s.set_z_order(id, 42);
    EXPECT_EQ(s.z_order(id), 42);
    s.set_z_order(id, -10);
    EXPECT_EQ(s.z_order(id), -10);
}

TEST(SceneTest, DefaultFlagsIsVisible) {
    scene s;
    auto pi = s.add_text(text_data{{{{"X", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);
    EXPECT_TRUE(s.is_visible(id));
    EXPECT_FALSE(s.is_focusable(id));
}

TEST(SceneTest, SetVisibleAndFocusable) {
    scene s;
    auto pi = s.add_text(text_data{{{{"X", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);

    s.set_focusable(id, true);
    EXPECT_TRUE(s.is_focusable(id));
    EXPECT_TRUE(s.is_visible(id));  // unchanged

    s.set_visible(id, false);
    EXPECT_FALSE(s.is_visible(id));
    EXPECT_TRUE(s.is_focusable(id));  // unchanged
}

// ── Clear (capacity reuse) ─────────────────────────────────────────────

TEST(SceneTest, ClearResetsButPreservesCapacity) {
    scene s;
    // Add several nodes
    for (int i = 0; i < 10; ++i) {
        auto pi = s.add_text(text_data{{{{"X", style{}}}}, overflow_mode::wrap, justify::left});
        s.add_node(widget_type::text, pi);
    }
    EXPECT_EQ(s.node_count(), 10u);

    s.clear();
    EXPECT_EQ(s.node_count(), 0u);

    // Can add nodes again after clear
    auto pi = s.add_text(text_data{{{{"Y", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);
    EXPECT_EQ(s.node_count(), 1u);
    EXPECT_EQ(s.z_order(id), 0);
    EXPECT_TRUE(s.is_visible(id));
}

// ── Render rect writeback ──────────────────────────────────────────────

TEST(SceneTest, RenderWritesBackRect) {
    scene s;
    auto pi = s.add_text(text_data{{{{"AB", style{}}}}, overflow_mode::wrap, justify::left});
    auto id = s.add_node(widget_type::text, pi);

    // Before render, area should be default (all zeros)
    EXPECT_EQ(s.area(id).x, 0u);
    EXPECT_EQ(s.area(id).w, 0u);

    canvas cv(10, 3);
    rect area{2, 1, 8, 2};
    dispatch_render(s, id, cv, area, s.styles());

    // After render, area should be written back
    EXPECT_EQ(s.area(id).x, 2u);
    EXPECT_EQ(s.area(id).y, 1u);
    EXPECT_EQ(s.area(id).w, 8u);
    EXPECT_EQ(s.area(id).h, 2u);
}

TEST(SceneTest, PanelWritesBackRectsForSelfAndChild) {
    scene s;
    auto pi_text = s.add_text(text_data{{{{"Hi", style{}}}}, overflow_mode::wrap, justify::left});
    auto pi_panel = s.add_panel(panel_data{no_node, border_style::ascii, style{}, "", style{}});
    auto panel_id = s.add_node(widget_type::panel, pi_panel);
    auto text_id = s.add_node(widget_type::text, pi_text, panel_id);
    s.get_panel(pi_panel).content = text_id;

    auto m = dispatch_measure(s, panel_id, box_constraints::loose(80, 24));
    canvas cv(m.width, m.height);
    rect area{0, 0, m.width, m.height};
    dispatch_render(s, panel_id, cv, area, s.styles());

    // Panel rect
    EXPECT_EQ(s.area(panel_id).x, 0u);
    EXPECT_EQ(s.area(panel_id).y, 0u);
    EXPECT_EQ(s.area(panel_id).w, m.width);
    EXPECT_EQ(s.area(panel_id).h, m.height);

    // Child text rect (inset by 1 for border)
    EXPECT_EQ(s.area(text_id).x, 1u);
    EXPECT_EQ(s.area(text_id).y, 1u);
    EXPECT_EQ(s.area(text_id).w, m.width - 2);
    EXPECT_EQ(s.area(text_id).h, m.height - 2);
}
