#pragma once

/**
 * @file live.h
 * @brief Live display engine: async render thread with fps control.
 *
 * Usage:
 *   tapiru::console con;
 *   {
 *       tapiru::live lv(con);
 *       lv.set(text_builder("[bold]Loading...[/]"));
 *       // ... update from any thread ...
 *       lv.set(text_builder("[green]Done![/]"));
 *   }  // destructor stops render thread and restores terminal
 */

#include "tapiru/core/canvas.h"
#include "tapiru/exports.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

namespace tapiru {

    class console;

    /**
     * @brief Live display that re-renders widgets at a fixed frame rate.
     *
     * The live display owns a render thread that periodically flattens the
     * current builder into a scene, measures, renders to canvas, diffs against
     * the previous frame, and emits only the changed cells.
     *
     * Thread-safe: `set()` can be called from any thread.
     */
    class TAPIRU_API live {
      public:
        /**
         * @brief Construct a live display attached to a console.
         * @param con     the console to render to (must outlive the live object)
         * @param fps     target frames per second (default 12)
         * @param width   output width (0 = auto-detect from terminal)
         */
        explicit live(console &con, uint32_t fps = 12, uint32_t width = 0);

        ~live();

        live(const live &) = delete;
        live &operator=(const live &) = delete;
        live(live &&) = delete;
        live &operator=(live &&) = delete;

        /**
         * @brief Set the widget to render. Thread-safe.
         *
         * The flatten function is stored and called on the render thread.
         * @tparam Builder any type with flatten_into(detail::scene&)
         */
        template <typename Builder> void set(Builder &&builder);

        /** @brief Stop the render thread and clear the live display. */
        void stop();

        /** @brief Force a full redraw on the next frame (e.g. after Ctrl+L or terminal corruption). */
        void force_redraw() noexcept { force_redraw_.store(true, std::memory_order_relaxed); }

        /** @brief Check if the live display is still running. */
        [[nodiscard]] bool running() const noexcept {
            return render_thread_.joinable() && !render_thread_.get_stop_token().stop_requested();
        }

      private:
        using flatten_fn = std::function<uint32_t(void *)>;

        void render_loop();
        void render_frame();

        console &console_;
        uint32_t width_;
        std::chrono::milliseconds frame_interval_;

        std::mutex mu_;
        flatten_fn pending_flatten_; // guarded by mu_
        bool dirty_ = false;         // guarded by mu_

        flatten_fn active_flatten_; // only accessed by render thread
        canvas canvas_;             // persistent double-buffered canvas
        uint32_t prev_height_ = 0;  // lines rendered in previous frame
        bool first_frame_ = true;   // first frame needs full output
        std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
        uint32_t frame_number_ = 0;

        std::atomic<bool> force_redraw_{false};
        std::jthread render_thread_;
    };

    // ── Template implementation ─────────────────────────────────────────────

    namespace detail {
        class scene;
    }

    template <typename Builder> void live::set(Builder &&builder) {
        auto shared = std::make_shared<std::decay_t<Builder>>(std::forward<Builder>(builder));
        std::lock_guard lk(mu_);
        pending_flatten_ = [shared](void *scene_ptr) -> uint32_t {
            auto &s = *static_cast<detail::scene *>(scene_ptr);
            return shared->flatten_into(s);
        };
        dirty_ = true;
    }

} // namespace tapiru
