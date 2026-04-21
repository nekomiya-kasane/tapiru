#pragma once

/**
 * @file keybinding.h
 * @brief Keybinding display widget — shows key combinations with styled badges.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    struct keybinding_entry {
        std::string keys;        // e.g. "Ctrl+S"
        std::string description; // e.g. "Save file"
    };

    class TAPIRU_API keybinding_builder {
      public:
        keybinding_builder() = default;

        keybinding_builder &add(std::string_view keys, std::string_view desc) {
            entries_.push_back({std::string(keys), std::string(desc)});
            return *this;
        }

        keybinding_builder &key_style(const style &s) {
            key_sty_ = s;
            return *this;
        }
        keybinding_builder &desc_style(const style &s) {
            desc_sty_ = s;
            return *this;
        }
        keybinding_builder &separator(std::string_view sep) {
            separator_ = sep;
            return *this;
        }
        keybinding_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }
        keybinding_builder &key(std::string_view k);

        node_id flatten_into(detail::scene &s) const;

      private:
        std::vector<keybinding_entry> entries_;
        style key_sty_;
        style desc_sty_;
        std::string separator_ = "  ";
        int16_t z_order_ = 0;
        uint64_t key_ = 0;
    };

} // namespace tapiru
