/**
 * @file tapiru.cppm
 * @brief C++20 module interface for tapiru.
 *
 * Usage:
 *   import tapiru;
 *
 * Requires CMake option TAPIRU_USE_MODULES=ON and a compiler with
 * C++20 module support (MSVC 17.4+, GCC 14+, Clang 16+).
 */

module;

// Pre-module includes (global module fragment)
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

export module tapiru;

// ── Core ────────────────────────────────────────────────────────────────

export import :core;

// ── Widgets ─────────────────────────────────────────────────────────────

export import :widgets;

// ── Text ────────────────────────────────────────────────────────────────

export import :text;
