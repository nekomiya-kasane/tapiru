#pragma once

/**
 * @file screen.h
 * @brief Screen runner: the sole entry point for rendering components.
 *
 * Three modes:
 *   1. fullscreen()       — interactive full-screen TUI (alternate screen buffer)
 *   2. fit_component()    — interactive, sized to component (inline)
 *   3. terminal_output()  — non-interactive streaming (piped/CI output)
 *
 * Usage:
 *   auto scr = screen::fullscreen();
 *   scr.loop(my_component);
 *
 *   auto scr = screen::terminal_output();
 *   scr.render_once(my_element);
 *   scr.set_live(my_component);  // live-updating spinner/progress
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "tapiru/core/component.h"
#include "tapiru/core/element.h"
#include "tapiru/exports.h"

namespace tapiru {

// ── screen_config ───────────────────────────────────────────────────────

struct screen_config {
    uint32_t fps = 30;
    uint32_t width = 0;       // 0 = auto-detect
    uint32_t height = 0;      // 0 = auto-detect (fullscreen only)
    bool     mouse = false;   // enable mouse tracking
    bool     cursor = false;  // show cursor

    /** @brief Custom output sink (default: stdout). */
    std::function<void(std::string_view)> sink;
};

// ── screen ──────────────────────────────────────────────────────────────

class TAPIRU_API screen {
public:
    ~screen();

    screen(const screen&) = delete;
    screen& operator=(const screen&) = delete;
    screen(screen&&) noexcept;
    screen& operator=(screen&&) noexcept;

    // ── Factory methods ─────────────────────────────────────────────────

    /** @brief Full-screen interactive mode (alternate screen buffer). */
    static screen fullscreen(screen_config cfg = {});

    /** @brief Inline interactive mode (sized to component). */
    static screen fit_component(screen_config cfg = {});

    /** @brief Non-interactive streaming output (for pipes/CI). */
    static screen terminal_output(screen_config cfg = {});

    // ── Interactive API ─────────────────────────────────────────────────

    /** @brief Run the main event loop with the given component. Blocks until exit. */
    void loop(component root);

    /** @brief Request the loop to exit. Thread-safe. */
    void exit_loop();

    /** @brief Post a task to run on the main loop thread. Thread-safe. */
    void post_event(input_event ev);

    // ── Streaming API ───────────────────────────────────────────────────

    /** @brief Render an element once and print to output. */
    void render_once(const element& elem);

    /** @brief Start live-updating display with a component (spinner/progress). */
    void set_live(component comp);

    /** @brief Stop the live display. */
    void stop_live();

    // ── Query ───────────────────────────────────────────────────────────

    /** @brief Whether the output is a TTY (interactive terminal). */
    [[nodiscard]] bool is_tty() const noexcept;

    /** @brief Whether this screen is in interactive mode. */
    [[nodiscard]] bool is_interactive() const noexcept;

    /** @brief Get the current terminal dimensions. */
    [[nodiscard]] std::pair<uint32_t, uint32_t> dimensions() const;

private:
    struct impl;
    std::unique_ptr<impl> impl_;

    explicit screen(std::unique_ptr<impl> p);
};

}  // namespace tapiru
