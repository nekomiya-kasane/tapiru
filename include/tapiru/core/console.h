#pragma once

/**
 * @file console.h
 * @brief Rich-like console for styled terminal output.
 *
 * Extends tapioca::basic_console with markup parsing, widget rendering,
 * syntax highlighting, and compile-time rich markup.
 *
 * Usage:
 *   tapiru::console con;
 *   con.print("[bold red]Error:[/] something went wrong");
 */

#include "tapioca/console.h"
#include "tapiru/core/ansi.h"
#include "tapiru/core/canvas.h"
#include "tapiru/core/cell.h"
#include "tapiru/core/style_table.h"
#include "tapiru/core/terminal.h"
#include "tapiru/exports.h"
#include "tapiru/text/render_op.h"

#include <cstdint>
#include <format>
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace tapiru {
    class highlighter;
}

namespace tapiru {

    // Re-export tapioca types into tapiru namespace for backward compatibility
    using tapioca::console_config;
    using tapioca::output_sink;

    /**
     * @brief Rich-like console for styled terminal output.
     *
     * Inherits basic output infrastructure from tapioca::basic_console.
     * Adds markup parsing, widget rendering, syntax highlighting,
     * and compile-time rich markup.
     */
    class TAPIRU_API console : public tapioca::basic_console {
      public:
        /** @brief Construct with default stdout sink and auto-detected capabilities.
         */
        console();

        /** @brief Construct with custom configuration. */
        explicit console(console_config cfg);

        /**
         * @brief Print a markup-formatted string, followed by a newline.
         *
         * Parses [tag] markup, resolves styles, and emits ANSI escape sequences.
         * @param markup  the input string with optional [tag] markup
         */
        void print(std::string_view markup);

        /** @brief Print with std::format-style arguments, followed by a newline. */
        template <typename... Args> void print(std::format_string<Args...> fmt, Args &&...args) {
            print(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

        /**
         * @brief Print a markup-formatted string without a trailing newline.
         */
        void print_inline(std::string_view markup);

        /** @brief Print inline with std::format-style arguments. */
        template <typename... Args> void print_inline(std::format_string<Args...> fmt, Args &&...args) {
            print_inline(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

        /**
         * @brief Render a widget builder and print the result.
         *
         * Flattens the builder into a scene, measures, renders to canvas,
         * then emits ANSI sequences to the sink.
         * @tparam Builder any type with a flatten_into(detail::scene&) method
         */
        template <typename Builder> void print_widget(const Builder &builder, uint32_t width = 0);

        /// @brief Render with bounded width AND height (enables flex layout).
        template <typename Builder> void print_widget(const Builder &builder, uint32_t width, uint32_t height);

        /**
         * @brief Print plain text with auto-highlighting applied, followed by a
         * newline.
         *
         * Uses the currently set highlighter (if any) to detect and style
         * patterns like URLs, numbers, paths, etc. No markup parsing.
         * @param text  plain text to highlight
         */
        void print_highlighted(std::string_view text);

        /** @brief Print highlighted with std::format-style arguments. */
        template <typename... Args> void print_highlighted(std::format_string<Args...> fmt, Args &&...args) {
            print_highlighted(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

        /** @brief Alias for print_highlighted. */
        void print_hl(std::string_view text) { print_highlighted(text); }

        /** @brief Alias for print_highlighted with std::format-style arguments. */
        template <typename... Args> void print_hl(std::format_string<Args...> fmt, Args &&...args) {
            print_highlighted(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

        /**
         * @brief Print plain text with auto-highlighting, no trailing newline.
         */
        void print_highlighted_inline(std::string_view text);

        /** @brief Print highlighted inline with std::format-style arguments. */
        template <typename... Args> void print_highlighted_inline(std::format_string<Args...> fmt, Args &&...args) {
            print_highlighted_inline(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

        /** @brief Alias for print_highlighted_inline. */
        void print_hl_inline(std::string_view text) { print_highlighted_inline(text); }

        /** @brief Alias for print_highlighted_inline with std::format-style
         * arguments. */
        template <typename... Args> void print_hl_inline(std::format_string<Args...> fmt, Args &&...args) {
            print_highlighted_inline(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

        /**
         * @brief Set the highlighter for auto-highlight mode.
         *
         * The highlighter is borrowed (caller owns lifetime).
         * Pass nullptr to disable auto-highlighting.
         */
        void set_highlighter(const highlighter *hl) noexcept;

        /**
         * @brief Enable/disable auto-highlighting on print() calls.
         *
         * When enabled, print() will apply the highlighter to plain-text
         * portions of the markup output. Requires a highlighter to be set.
         */
        void set_auto_highlight(bool enabled) noexcept;

        /** @brief Check if auto-highlighting is enabled. */
        [[nodiscard]] bool auto_highlight() const noexcept { return auto_highlight_; }

        /** @brief Get the current highlighter (may be nullptr). */
        [[nodiscard]] const highlighter *get_highlighter() const noexcept { return highlighter_; }

        // Bring base class write/newline into this scope (template overload would hide them)
        using basic_console::newline;
        using basic_console::write;

        /** @brief Write with std::format-style arguments (no markup, no newline). */
        template <typename... Args> void write(std::format_string<Args...> fmt, Args &&...args) {
            basic_console::write(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

        /**
         * @brief Execute a pre-compiled rich markup plan.
         *
         * Typically called via the TAPIRU_PRINT_RICH macro which
         * compiles the markup at compile time.
         */
        template <size_t MaxFragments>
        void emit_rich_plan(const static_rich_markup<MaxFragments> &plan, const char *src);

        /**
         * @brief Print rich markup (runtime path).
         *
         * Parses block-level tags at runtime and renders them.
         */
        void print_rich(std::string_view markup);

        /** @brief Print rich markup with std::format-style arguments. */
        template <typename... Args> void print_rich(std::format_string<Args...> fmt, Args &&...args) {
            print_rich(std::string_view{std::format(fmt, std::forward<Args>(args)...)});
        }

      private:
        void emit_fragments(std::string_view markup, bool append_newline);
        void emit_highlighted_fragments(std::string_view text, bool append_newline);
        void render_widget_impl(const std::function<uint32_t(void *)> &flatten_fn, void *builder_ptr, uint32_t width);
        void render_widget_impl(const std::function<uint32_t(void *)> &flatten_fn, void *builder_ptr, uint32_t width,
                                uint32_t height);

        const highlighter *highlighter_ = nullptr;
        bool auto_highlight_ = false;
    };

    // ── Canvas capture ──────────────────────────────────────────────────────

    namespace detail {
        class scene;
    }

    struct TAPIRU_API widget_canvas {
        canvas cv;
        style_table styles;
        uint32_t width = 0;
        uint32_t height = 0;

        /** @brief Get the codepoint at (x, y). Returns U' ' for empty/null cells. */
        [[nodiscard]] char32_t char_at(uint32_t x, uint32_t y) const {
            if (x >= width || y >= height) {
                return U' ';
            }
            auto cp = cv.get(x, y).codepoint;
            return (cp == 0) ? U' ' : cp;
        }

        /** @brief Get the cell at (x, y). */
        [[nodiscard]] const cell &cell_at(uint32_t x, uint32_t y) const { return cv.get(x, y); }

        /** @brief Get the style of the cell at (x, y). */
        [[nodiscard]] style style_at(uint32_t x, uint32_t y) const { return styles.lookup(cv.get(x, y).sid); }

        /** @brief Extract plain-text row as UTF-8 string. */
        [[nodiscard]] std::string row_text(uint32_t y) const;

        /** @brief Extract all rows as plain-text strings. */
        [[nodiscard]] std::vector<std::string> all_rows() const;

        /** @brief Dump canvas chars to an output stream with row numbers. */
        void dump(std::ostream &os) const;

        /** @brief Dump with style info (fg/bg RGB) for each cell. */
        void dump_styled(std::ostream &os) const;
    };

    /**
     * @brief Render a widget builder to a canvas without converting to ANSI.
     *
     * Runs the full pipeline: flatten → measure → render → border_join,
     * then returns the raw canvas for inspection.
     */
    TAPIRU_API widget_canvas render_to_canvas_impl(const std::function<uint32_t(void *)> &flatten_fn, uint32_t width);

    template <typename Builder> widget_canvas render_to_canvas(const Builder &builder, uint32_t width) {
        return render_to_canvas_impl(
            [&builder](void *scene_ptr) -> uint32_t {
                auto &s = *static_cast<detail::scene *>(scene_ptr);
                return builder.flatten_into(s);
            },
            width);
    }

    // ── Template implementation ─────────────────────────────────────────────

    template <typename Builder> void console::print_widget(const Builder &builder, uint32_t width) {
        auto flatten_fn = [](void *ptr) -> uint32_t {
            // This is a type-erased trampoline; actual scene is created inside
            // render_widget_impl
            (void)ptr;
            return 0;
        };
        // Use a different approach: pass a lambda that captures the builder
        render_widget_impl(
            [&builder](void *scene_ptr) -> uint32_t {
                auto &s = *static_cast<detail::scene *>(scene_ptr);
                return builder.flatten_into(s);
            },
            nullptr, width);
    }

    template <typename Builder> void console::print_widget(const Builder &builder, uint32_t width, uint32_t height) {
        render_widget_impl(
            [&builder](void *scene_ptr) -> uint32_t {
                auto &s = *static_cast<detail::scene *>(scene_ptr);
                return builder.flatten_into(s);
            },
            nullptr, width, height);
    }

    template <size_t MaxFragments>
    void console::emit_rich_plan(const static_rich_markup<MaxFragments> &plan, const char *src) {
        std::string buf;
        buf.reserve(256);

        markup_render_context ctx;
        ctx.src = src;
        ctx.width = term_width();
        ctx.color_on = color_enabled();
        ctx.emitter = &emitter_;
        ctx.out = &buf;

        execute_rich(plan, ctx);

        if (color_enabled()) {
            emitter_.reset(buf);
        }

        buf += '\n';
        config_.sink(buf);
    }

/**
 * @brief Print rich markup with compile-time parsing.
 *
 * Usage: TAPIRU_PRINT_RICH(con, "[box]Hello[/box]");
 */
#define TAPIRU_PRINT_RICH(console_obj, literal)                                                                        \
    do {                                                                                                               \
        constexpr auto _tapiru_plan_ = ::tapiru::compile_markup(literal);                                              \
        (console_obj).emit_rich_plan(_tapiru_plan_, literal);                                                          \
    } while (0)

} // namespace tapiru

/**
 * @brief Global default console instance for quick usage.
 *
 * Uses default stdout sink with auto-detected terminal capabilities.
 * Available without any namespace qualifier:
 *   tcon.print("[bold]hello[/]");
 *   tcon.print_hl("count {}", 42);
 */
extern TAPIRU_API tapiru::console tcon;
