/**
 * @file screen.cpp
 * @brief Screen runner implementation: fullscreen, fit_component, terminal_output.
 */

#include "tapiru/core/screen.h"

#include "tapiru/core/console.h"
#include "tapiru/core/live.h"
#include "tapiru/core/terminal.h"

#include <mutex>
#include <queue>
#include <thread>

namespace tapiru {

    // ── screen mode enum ────────────────────────────────────────────────────

    enum class screen_mode : uint8_t {
        fullscreen,
        fit_component,
        terminal_output,
    };

    // ── screen::impl ────────────────────────────────────────────────────────

    struct screen::impl {
        screen_mode mode;
        screen_config cfg;
        console con;
        std::unique_ptr<live> live_display;

        std::atomic<bool> exit_requested{false};

        std::mutex event_mu;
        std::queue<input_event> event_queue; // guarded by event_mu

        impl(screen_mode m, screen_config c) : mode(m), cfg(std::move(c)) {
            console_config cc;
            if (cfg.sink) {
                cc.sink = cfg.sink;
            }
            cc.depth = terminal::detect_color_depth();
            if (terminal::is_tty()) {
                cc.force_color = true;
            }
            con = console(cc);
        }

        ~impl() {
            if (live_display) {
                live_display->stop();
            }
        }

        uint32_t output_width() const {
            if (cfg.width > 0) {
                return cfg.width;
            }
            auto sz = terminal::get_size();
            return sz.width > 0 ? sz.width : 80;
        }

        uint32_t output_height() const {
            if (cfg.height > 0) {
                return cfg.height;
            }
            auto sz = terminal::get_size();
            return sz.height > 0 ? sz.height : 24;
        }
    };

    // ── screen construction / destruction ───────────────────────────────────

    screen::screen(std::unique_ptr<impl> p) : impl_(std::move(p)) {}
    screen::~screen() = default;
    screen::screen(screen &&) noexcept = default;
    screen &screen::operator=(screen &&) noexcept = default;

    screen screen::fullscreen(screen_config cfg) {
        return screen(std::make_unique<impl>(screen_mode::fullscreen, std::move(cfg)));
    }

    screen screen::fit_component(screen_config cfg) {
        return screen(std::make_unique<impl>(screen_mode::fit_component, std::move(cfg)));
    }

    screen screen::terminal_output(screen_config cfg) {
        return screen(std::make_unique<impl>(screen_mode::terminal_output, std::move(cfg)));
    }

    // ── Interactive API ─────────────────────────────────────────────────────

    void screen::loop(component root) {
        if (!impl_ || !root) {
            return;
        }

        auto &d = *impl_;
        d.exit_requested.store(false);

        const auto frame_interval = std::chrono::milliseconds(1000 / std::max(d.cfg.fps, 1u));

        // Enter alternate screen for fullscreen mode
        if (d.mode == screen_mode::fullscreen) {
            d.con.write("\x1b[?1049h"); // enter alternate screen
            if (!d.cfg.cursor) {
                d.con.write("\x1b[?25l"); // hide cursor
            }
            if (d.cfg.mouse) {
                d.con.write("\x1b[?1000h\x1b[?1006h"); // enable mouse
            }
        } else if (d.mode == screen_mode::fit_component) {
            if (!d.cfg.cursor) {
                d.con.write("\x1b[?25l");
            }
        }

        // Main loop
        while (!d.exit_requested.load(std::memory_order_relaxed)) {
            // Process queued events
            {
                std::lock_guard lk(d.event_mu);
                while (!d.event_queue.empty()) {
                    auto ev = d.event_queue.front();
                    d.event_queue.pop();
                    root->on_event(ev);
                }
            }

            // Render
            element elem = root->render();
            d.con.print_widget(elem, d.output_width());

            // Sleep for frame interval
            std::this_thread::sleep_for(frame_interval);

            // For fit_component mode, move cursor up to overwrite previous frame
            if (d.mode == screen_mode::fit_component) {
                // We'd need to track the number of lines rendered to move up.
                // For now, this is a simplified version.
            }
        }

        // Cleanup
        if (d.mode == screen_mode::fullscreen) {
            if (d.cfg.mouse) {
                d.con.write("\x1b[?1006l\x1b[?1000l");
            }
            d.con.write("\x1b[?25h");   // show cursor
            d.con.write("\x1b[?1049l"); // leave alternate screen
        } else if (d.mode == screen_mode::fit_component) {
            d.con.write("\x1b[?25h");
        }
    }

    void screen::exit_loop() {
        if (impl_) {
            impl_->exit_requested.store(true, std::memory_order_relaxed);
        }
    }

    void screen::post_event(input_event ev) {
        if (!impl_) {
            return;
        }
        std::lock_guard lk(impl_->event_mu);
        impl_->event_queue.push(std::move(ev));
    }

    // ── Streaming API ───────────────────────────────────────────────────────

    void screen::render_once(const element &elem) {
        if (!impl_) {
            return;
        }
        impl_->con.print_widget(elem, impl_->output_width());
    }

    void screen::set_live(component comp) {
        if (!impl_ || !comp) {
            return;
        }
        // Stop any existing live display
        stop_live();
        // Create a new live display that renders the component
        impl_->live_display = std::make_unique<live>(impl_->con, impl_->cfg.fps, impl_->output_width());
        // Wrap the component's render() into a builder with flatten_into for live::set()
        struct component_adapter {
            component comp_;
            node_id flatten_into(detail::scene &s) const { return comp_->render().flatten_into(s); }
        };
        impl_->live_display->set(component_adapter{comp});
    }

    void screen::stop_live() {
        if (impl_ && impl_->live_display) {
            impl_->live_display->stop();
            impl_->live_display.reset();
        }
    }

    // ── Query ───────────────────────────────────────────────────────────────

    bool screen::is_tty() const noexcept {
        return terminal::is_tty();
    }

    bool screen::is_interactive() const noexcept {
        if (!impl_) {
            return false;
        }
        return impl_->mode != screen_mode::terminal_output;
    }

    std::pair<uint32_t, uint32_t> screen::dimensions() const {
        if (!impl_) {
            return {80, 24};
        }
        return {impl_->output_width(), impl_->output_height()};
    }

} // namespace tapiru
