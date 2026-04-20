/**
 * @file live.cpp
 * @brief Live display engine implementation.
 */

#include "tapiru/core/live.h"

#include "detail/border_join.h"
#include "detail/scene.h"
#include "detail/widget_registry.h"
#include "tapiru/core/canvas.h"
#include "tapiru/core/console.h"
#include "tapiru/core/terminal.h"
#include "tapiru/core/unicode_width.h"
#include "tapiru/layout/types.h"

#include <string>

namespace tapiru {

namespace {

void append_utf8(std::string &out, char32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

/** @brief Emit a single cell's codepoint to the output buffer. */
void emit_cell(std::string &out, const cell &c) {
    if (c.codepoint == 0 || c.codepoint == U' ') {
        out += ' ';
    } else {
        append_utf8(out, c.codepoint);
    }
}

} // anonymous namespace

// ── Constructor / Destructor ────────────────────────────────────────────

live::live(console &con, uint32_t fps, uint32_t width)
    : console_(con), width_(width),
      frame_interval_(fps > 0 ? std::chrono::milliseconds(1000 / fps) : std::chrono::milliseconds(83)) {
    if (width_ == 0) {
        auto sz = terminal::get_size();
        width_ = sz.width > 0 ? sz.width : 80;
    }

    // Hide cursor
    console_.write("\x1b[?25l");

    render_thread_ = std::jthread([this](std::stop_token st) {
        while (!st.stop_requested()) {
            render_frame();
            std::this_thread::sleep_for(frame_interval_);
        }
    });
}

live::~live() {
    stop();
}

void live::stop() {
    if (!render_thread_.joinable()) {
        return; // already stopped
    }

    render_thread_.request_stop();
    render_thread_.join();

    // Render one final frame
    render_frame();

    // Show cursor
    console_.write("\x1b[?25h");
}

// ── Render loop ─────────────────────────────────────────────────────────

void live::render_frame() {
    // Swap in pending flatten function if dirty
    {
        std::lock_guard lk(mu_);
        if (dirty_) {
            active_flatten_ = std::move(pending_flatten_);
            dirty_ = false;
        }
    }

    if (!active_flatten_) return;

    // 1. Flatten + measure
    detail::scene sc;
    auto root_id = active_flatten_(&sc);

    auto bc = box_constraints::loose(width_, unbounded);
    auto m = detail::dispatch_measure(sc, root_id, bc);

    // 2. Handle canvas resize if dimensions changed
    if (m.width != canvas_.width() || m.height != canvas_.height()) {
        canvas_.resize(m.width, m.height);
        first_frame_ = true; // force full output after resize
    }

    // Handle force_redraw request
    if (force_redraw_.exchange(false, std::memory_order_relaxed)) {
        canvas_.invalidate_current();
        first_frame_ = true;
    }

    // 3. Copy current buffer forward, then render into it (dirty-rect optimization)
    canvas_.copy_current_to_next();
    rect area{0, 0, m.width, m.height};

    auto now = std::chrono::steady_clock::now();
    double frame_time = std::chrono::duration<double>(now - start_time_).count();
    detail::render_context rctx{sc, canvas_, sc.styles(), frame_time, frame_number_++};
    detail::dispatch_render(rctx, root_id, area);

    // 3b. Post-process: merge adjacent border characters
    detail::apply_border_joins(canvas_);

    // 4. Build output using diff-based cursor-addressing
    std::string buf;
    buf.reserve(m.width * m.height * 2 + 256);

    bool use_color = console_.color_enabled();
    ansi_emitter emitter;

    if (first_frame_) {
        // First frame or after resize/force_redraw: absolute position to top-left
        buf += "\x1b[H"; // CUP: cursor to row 1, col 1

        for (uint32_t y = 0; y < m.height; ++y) {
            buf += "\x1b[2K";
            for (uint32_t x = 0; x < m.width; ++x) {
                const auto &c = canvas_.get(x, y);
                if (c.width == 0) continue;
                if (use_color) {
                    emitter.transition(sc.styles().lookup(c.sid), buf);
                }
                emit_cell(buf, c);
            }
            if (use_color) emitter.reset(buf);
            if (y + 1 < m.height) buf += '\n';
        }

        // Clear leftover lines from previous taller frame
        for (uint32_t y = m.height; y < prev_height_; ++y) {
            buf += "\n\x1b[2K";
        }

        first_frame_ = false;
    } else {
        // Diff-based output: first check if anything changed at all
        bool any_changes = false;
        for (uint32_t y = 0; y < m.height && !any_changes; ++y) {
            for (uint32_t x = 0; x < m.width && !any_changes; ++x) {
                if (!(canvas_.get(x, y) == canvas_.get_current(x, y))) {
                    any_changes = true;
                }
            }
        }

        // Also need output if frame height changed (to clear leftover lines)
        if (!any_changes && m.height == prev_height_) {
            // Nothing changed — skip output entirely, but still swap buffers
            canvas_.swap();
            return;
        }

        // Absolute position to top-left
        buf += "\x1b[H";

        for (uint32_t y = 0; y < m.height; ++y) {
            bool line_has_changes = false;
            uint32_t x = 0;

            while (x < m.width) {
                // Skip unchanged cells
                if (canvas_.get(x, y) == canvas_.get_current(x, y)) {
                    ++x;
                    continue;
                }

                // Found a changed cell — start a run
                uint32_t run_start = x;

                // Collect run: include changed cells and bridge gaps of ≤2 unchanged cells
                while (x < m.width) {
                    if (canvas_.get(x, y) != canvas_.get_current(x, y)) {
                        ++x;
                    } else {
                        uint32_t gap = 0;
                        uint32_t peek = x;
                        while (peek < m.width && gap < 3 && canvas_.get(peek, y) == canvas_.get_current(peek, y)) {
                            ++peek;
                            ++gap;
                        }
                        if (peek < m.width && gap <= 2 && canvas_.get(peek, y) != canvas_.get_current(peek, y)) {
                            x = peek; // bridge the gap
                        } else {
                            break; // end of run
                        }
                    }
                }

                // Move cursor to absolute position (row y+1, col run_start+1, both 1-based)
                buf += "\x1b[";
                buf += std::to_string(y + 1);
                buf += ';';
                buf += std::to_string(run_start + 1);
                buf += 'H'; // CUP: Cursor Position (row;col)

                for (uint32_t rx = run_start; rx < x; ++rx) {
                    const auto &c = canvas_.get(rx, y);
                    if (c.width == 0) continue;
                    if (use_color) {
                        emitter.transition(sc.styles().lookup(c.sid), buf);
                    }
                    emit_cell(buf, c);
                }
                line_has_changes = true;
            }

            (void)line_has_changes;
        }

        if (use_color) emitter.reset(buf);

        // If frame is shorter than previous, clear leftover lines
        for (uint32_t y = m.height; y < prev_height_; ++y) {
            buf += "\x1b[";
            buf += std::to_string(y + 1);
            buf += ";1H\x1b[2K";
        }
    }

    prev_height_ = m.height;

    // 5. Swap buffers: current ← next (O(1) pointer swap)
    canvas_.swap();

    console_.write(buf);
}

} // namespace tapiru
