#pragma once

/**
 * @file toast.h
 * @brief Toast notification widget — a temporary message overlay.
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/layout/types.h"

#include <chrono>
#include <string>
#include <string_view>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    enum class toast_level : uint8_t { info, success, warning, error };

    class TAPIRU_API toast_state {
      public:
        toast_state() = default;

        void show(std::string_view message, toast_level level = toast_level::info,
                  std::chrono::milliseconds duration = std::chrono::milliseconds(3000)) {
            message_ = message;
            level_ = level;
            remaining_ = duration;
            visible_ = true;
        }

        void tick(std::chrono::milliseconds dt) {
            if (!visible_) {
                return;
            }
            if (remaining_ <= dt) {
                visible_ = false;
                remaining_ = std::chrono::milliseconds(0);
            } else {
                remaining_ -= dt;
            }
        }

        void dismiss() { visible_ = false; }

        [[nodiscard]] bool visible() const noexcept { return visible_; }
        [[nodiscard]] const std::string &message() const noexcept { return message_; }
        [[nodiscard]] toast_level level() const noexcept { return level_; }

      private:
        std::string message_;
        toast_level level_ = toast_level::info;
        std::chrono::milliseconds remaining_{0};
        bool visible_ = false;
    };

    class TAPIRU_API toast_builder {
      public:
        toast_builder() = default;

        toast_builder &state(const toast_state *s) {
            state_ = s;
            return *this;
        }
        toast_builder &info_style(const style &s) {
            info_sty_ = s;
            return *this;
        }
        toast_builder &success_style(const style &s) {
            success_sty_ = s;
            return *this;
        }
        toast_builder &warning_style(const style &s) {
            warning_sty_ = s;
            return *this;
        }
        toast_builder &error_style(const style &s) {
            error_sty_ = s;
            return *this;
        }
        toast_builder &border(border_style bs) {
            border_ = bs;
            return *this;
        }
        toast_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }
        toast_builder &key(std::string_view k);

        node_id flatten_into(detail::scene &s) const;

      private:
        const toast_state *state_ = nullptr;
        style info_sty_;
        style success_sty_;
        style warning_sty_;
        style error_sty_;
        border_style border_ = border_style::rounded;
        int16_t z_order_ = 10;
        uint64_t key_ = 0;
    };

} // namespace tapiru
