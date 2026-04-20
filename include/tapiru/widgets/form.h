#pragma once

/**
 * @file form.h
 * @brief Form component with field validation.
 *
 * Provides a multi-field form with per-field validation, Tab navigation,
 * and submit/cancel actions. Each field renders as a labeled text input
 * with optional error message display.
 */

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "tapiru/core/component.h"
#include "tapiru/core/style.h"
#include "tapiru/exports.h"

namespace tapiru {

// ── form_option ─────────────────────────────────────────────────────────

struct form_option {
    struct field {
        std::string label;
        std::string* value = nullptr;
        std::function<bool(std::string_view)> validator;
        std::string error_message;
    };

    std::vector<field> fields;
    std::function<void()> on_submit;

    // Optional styling
    style label_sty;
    style input_sty;
    style error_sty;
    style focused_sty;
    uint32_t input_width = 30;
};

TAPIRU_API component make_form(form_option opt);

}  // namespace tapiru
