/**
 * @file accessibility_test.cpp
 * @brief Tests for WAI-ARIA role annotations and accessibility properties.
 */

#include "tapiru/core/accessibility.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── aria_role enum ──────────────────────────────────────────────────────

TEST(AccessibilityTest, RoleNoneIsZero) {
    EXPECT_EQ(static_cast<uint8_t>(aria_role::none), 0u);
}

TEST(AccessibilityTest, WidgetRolesDistinct) {
    EXPECT_NE(aria_role::button, aria_role::checkbox);
    EXPECT_NE(aria_role::dialog, aria_role::menu);
    EXPECT_NE(aria_role::textbox, aria_role::slider);
}

TEST(AccessibilityTest, LandmarkRolesDistinct) {
    EXPECT_NE(aria_role::banner, aria_role::navigation);
    EXPECT_NE(aria_role::main, aria_role::complementary);
    EXPECT_NE(aria_role::form, aria_role::search);
}

// ── a11y_props defaults ─────────────────────────────────────────────────

TEST(AccessibilityTest, PropsDefaultValues) {
    a11y_props props;
    EXPECT_EQ(props.role, aria_role::none);
    EXPECT_TRUE(props.label.empty());
    EXPECT_TRUE(props.description.empty());
    EXPECT_EQ(props.heading_level, 0u);
    EXPECT_FALSE(props.expanded);
    EXPECT_FALSE(props.selected);
    EXPECT_FALSE(props.disabled);
    EXPECT_FALSE(props.checked);
    EXPECT_FALSE(props.live);
    EXPECT_FALSE(props.atomic);
}

TEST(AccessibilityTest, PropsDesignatedInit) {
    a11y_props props{
        .role = aria_role::button,
        .label = "Submit",
        .description = "Submit the form",
        .disabled = true,
    };
    EXPECT_EQ(props.role, aria_role::button);
    EXPECT_EQ(props.label, "Submit");
    EXPECT_EQ(props.description, "Submit the form");
    EXPECT_TRUE(props.disabled);
    EXPECT_FALSE(props.checked);
}

TEST(AccessibilityTest, PropsCheckbox) {
    a11y_props props{
        .role = aria_role::checkbox,
        .label = "Accept terms",
        .checked = true,
    };
    EXPECT_EQ(props.role, aria_role::checkbox);
    EXPECT_TRUE(props.checked);
}

TEST(AccessibilityTest, PropsHeading) {
    a11y_props props{
        .role = aria_role::heading,
        .label = "Section Title",
        .heading_level = 2,
    };
    EXPECT_EQ(props.role, aria_role::heading);
    EXPECT_EQ(props.heading_level, 2u);
}

TEST(AccessibilityTest, PropsLiveRegion) {
    a11y_props props{
        .role = aria_role::alert,
        .label = "Error notification",
        .live = true,
        .atomic = true,
    };
    EXPECT_EQ(props.role, aria_role::alert);
    EXPECT_TRUE(props.live);
    EXPECT_TRUE(props.atomic);
}

// ── role_name utility ───────────────────────────────────────────────────

TEST(AccessibilityTest, RoleNameNone) {
    EXPECT_EQ(role_name(aria_role::none), "none");
}

TEST(AccessibilityTest, RoleNameButton) {
    EXPECT_EQ(role_name(aria_role::button), "button");
}

TEST(AccessibilityTest, RoleNameNavigation) {
    EXPECT_EQ(role_name(aria_role::navigation), "navigation");
}

TEST(AccessibilityTest, RoleNameTextbox) {
    EXPECT_EQ(role_name(aria_role::textbox), "textbox");
}

TEST(AccessibilityTest, RoleNameProgressbar) {
    EXPECT_EQ(role_name(aria_role::progressbar), "progressbar");
}

TEST(AccessibilityTest, RoleNameAlert) {
    EXPECT_EQ(role_name(aria_role::alert), "alert");
}

TEST(AccessibilityTest, RoleNameTable) {
    EXPECT_EQ(role_name(aria_role::table), "table");
}

// ── accessible_component mixin ──────────────────────────────────────────

class test_button : public accessible_component {
  public:
    [[nodiscard]] a11y_props get_a11y_props() const override {
        return {.role = aria_role::button, .label = std::string(a11y_label())};
    }
};

TEST(AccessibilityTest, ComponentDefaultProps) {
    accessible_component comp;
    auto props = comp.get_a11y_props();
    EXPECT_EQ(props.role, aria_role::none);
}

TEST(AccessibilityTest, ComponentSetLabel) {
    accessible_component comp;
    comp.set_a11y_label("OK");
    EXPECT_EQ(comp.a11y_label(), "OK");
}

TEST(AccessibilityTest, DerivedComponentProps) {
    test_button btn;
    btn.set_a11y_label("Cancel");
    auto props = btn.get_a11y_props();
    EXPECT_EQ(props.role, aria_role::button);
    EXPECT_EQ(props.label, "Cancel");
}

// ── a11y_node struct ────────────────────────────────────────────────────

TEST(AccessibilityTest, A11yNodeDefaults) {
    a11y_node node;
    EXPECT_EQ(node.role, aria_role::none);
    EXPECT_TRUE(node.label.empty());
    EXPECT_TRUE(node.description.empty());
    EXPECT_EQ(node.depth, 0u);
}

TEST(AccessibilityTest, A11yNodeCustom) {
    a11y_node node{aria_role::menu, "File Menu", "Main file operations menu", 1};
    EXPECT_EQ(node.role, aria_role::menu);
    EXPECT_EQ(node.label, "File Menu");
    EXPECT_EQ(node.depth, 1u);
}
