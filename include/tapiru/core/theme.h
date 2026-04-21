#pragma once

/**
 * @file theme.h
 * @brief Theme/style sheet system for tapiru.
 *
 * Allows defining named styles and applying them via markup:
 *   theme th = theme::dark();
 *   th.define("danger", style{}.fg(color::from_rgb(255,0,0)).bold());
 *   con.set_theme(th);
 *   con.print_widget(text_builder("[.danger]Error![/]"), 80);
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace tapiru {

    /**
     * @brief A collection of named styles that can be referenced in markup.
     */
    class TAPIRU_API theme {
      public:
        theme() = default;

        void define(std::string name, const style &s);
        [[nodiscard]] const style *lookup(std::string_view name) const;
        [[nodiscard]] bool has(std::string_view name) const;
        void remove(std::string_view name);
        void clear();

        [[nodiscard]] size_t size() const noexcept { return styles_.size(); }

        // ── Preset themes ──────────────────────────────────────────────────

        [[nodiscard]] static theme dark();
        [[nodiscard]] static theme light();
        [[nodiscard]] static theme monokai();

      private:
        std::unordered_map<std::string, style> styles_;
    };

} // namespace tapiru
