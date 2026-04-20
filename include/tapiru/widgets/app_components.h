#pragma once

/**
 * @file app_components.h
 * @brief Application-level component factories: menu_bar, status_bar,
 *        resizable_split, window.
 *
 * These replace the old classic_app monolith with composable components.
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "tapiru/core/component.h"
#include "tapiru/core/element.h"
#include "tapiru/exports.h"
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/menu_bar.h"

namespace tapiru {

// ── menu_bar component ──────────────────────────────────────────────────

TAPIRU_API component make_menu_bar(std::vector<menu_bar_entry> entries, int* selected);

// ── status_bar component ────────────────────────────────────────────────

TAPIRU_API component make_status_bar(std::function<element()> content_fn);

// ── resizable_split ─────────────────────────────────────────────────────

TAPIRU_API component resizable_split_left(component left, component right, int* split_pos);
TAPIRU_API component resizable_split_top(component top, component bottom, int* split_pos);

}  // namespace tapiru
