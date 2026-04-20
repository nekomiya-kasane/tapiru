#pragma once

/**
 * @file component_factories.h
 * @brief Component factory functions wrapping existing interactive builders.
 *
 * Each factory returns a component (shared_ptr<component_base>) that handles
 * events internally and produces elements via the underlying builder.
 */

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "tapiru/core/component.h"
#include "tapiru/core/style.h"
#include "tapiru/exports.h"

namespace tapiru {

// ── button ──────────────────────────────────────────────────────────────

struct button_option {
    std::string label;
    std::function<void()> on_click;
    style sty;
};

TAPIRU_API component make_button(button_option opt);

// ── checkbox ────────────────────────────────────────────────────────────

struct checkbox_option {
    std::string label;
    bool* value = nullptr;
    style sty;
};

TAPIRU_API component make_checkbox(checkbox_option opt);

// ── radio_group ─────────────────────────────────────────────────────────

struct radio_group_option {
    std::vector<std::string> options;
    int* selected = nullptr;
    style sty;
};

TAPIRU_API component make_radio_group(radio_group_option opt);

// ── selectable_list ─────────────────────────────────────────────────────

struct selectable_list_option {
    std::vector<std::string> items;
    int* cursor = nullptr;
    uint32_t visible_count = 0;
    style sty;
    style highlight_sty;
    std::function<void(int)> on_select;
};

TAPIRU_API component make_selectable_list(selectable_list_option opt);

// ── text_input ──────────────────────────────────────────────────────────

struct text_input_option {
    std::string* buffer = nullptr;
    std::string placeholder;
    uint32_t width = 20;
    style sty;
    std::function<void(const std::string&)> on_change;
};

TAPIRU_API component make_text_input(text_input_option opt);

// ── slider ──────────────────────────────────────────────────────────────

struct slider_option {
    float* value = nullptr;
    float min_val = 0.0f;
    float max_val = 1.0f;
    float step = 0.01f;
    uint32_t width = 20;
    style sty;
    std::function<void(float)> on_change;
};

TAPIRU_API component make_slider(slider_option opt);

}  // namespace tapiru
