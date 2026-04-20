#pragma once

/**
 * @file accessibility.h
 * @brief Accessibility role annotations for tapiru elements and components.
 *
 * Provides WAI-ARIA-inspired semantic roles for TUI widgets. These roles
 * allow screen readers, automated testing frameworks, and accessibility
 * auditing tools to understand the purpose of UI elements.
 *
 * Usage:
 *   auto btn = text_builder("OK") | role(aria_role::button, "Confirm action");
 *   auto nav = container::vertical({...}) | role(aria_role::navigation, "Main menu");
 */

#include <cstdint>
#include <string>
#include <string_view>

#include "tapiru/core/element.h"
#include "tapiru/exports.h"

namespace tapiru {

// ── Accessibility roles (WAI-ARIA subset for TUI) ────────────────────

enum class aria_role : uint8_t {
    none = 0,

    // Landmark roles
    banner,
    navigation,
    main,
    complementary,
    contentinfo,
    form,
    search,
    region,

    // Widget roles
    button,
    checkbox,
    dialog,
    link,
    listbox,
    list_item,
    menu,
    menu_item,
    option,
    progressbar,
    radio,
    scrollbar,
    separator,
    slider,
    spinbutton,
    status,
    tab,
    tab_panel,
    textbox,
    toolbar,
    tooltip,
    tree,
    tree_item,

    // Live region roles
    alert,
    log,
    marquee,
    timer,

    // Document structure
    heading,
    img,
    table,
    row,
    cell,
    group,
};

// ── Accessibility properties ─────────────────────────────────────────

struct a11y_props {
    aria_role   role        = aria_role::none;
    std::string label;          // human-readable label (aria-label)
    std::string description;    // extended description (aria-describedby)
    uint8_t     heading_level = 0;  // 1-6 for aria_role::heading
    bool        expanded      = false;  // aria-expanded (for tree/menu)
    bool        selected      = false;  // aria-selected
    bool        disabled      = false;  // aria-disabled
    bool        checked       = false;  // aria-checked (for checkbox/radio)
    bool        live          = false;  // aria-live="polite" (announces changes)
    bool        atomic        = false;  // aria-atomic (announce entire region)
};

// ── Role decorator ───────────────────────────────────────────────────

/** @brief Attach an accessibility role and label to an element. */
TAPIRU_API decorator role(aria_role r, std::string_view label = {});

/** @brief Attach full accessibility properties to an element. */
TAPIRU_API decorator accessible(a11y_props props);

// ── Component mixin ──────────────────────────────────────────────────

/**
 * @brief Mixin interface for components that provide accessibility info.
 *
 * Components can inherit from this to expose their role and label
 * to the accessibility tree traversal.
 */
class TAPIRU_API accessible_component {
public:
    virtual ~accessible_component() = default;

    /** @brief Get the accessibility properties of this component. */
    [[nodiscard]] virtual a11y_props get_a11y_props() const {
        return {.role = aria_role::none};
    }

    /** @brief Set the accessibility label. */
    void set_a11y_label(std::string_view label) { a11y_label_ = label; }

    /** @brief Get the accessibility label. */
    [[nodiscard]] std::string_view a11y_label() const noexcept { return a11y_label_; }

protected:
    std::string a11y_label_;
};

// ── Accessibility tree query ─────────────────────────────────────────

struct a11y_node {
    aria_role   role  = aria_role::none;
    std::string label;
    std::string description;
    uint32_t    depth = 0;
};

/** @brief Convert an aria_role to its string name. */
[[nodiscard]] TAPIRU_API std::string_view role_name(aria_role r) noexcept;

}  // namespace tapiru
