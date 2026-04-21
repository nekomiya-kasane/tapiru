#pragma once

/**
 * @file interactive.h
 * @brief Interactive widget builders for retained-mode terminal UIs.
 *
 * All components bind to external state via pointers/references.
 * Users update state in response to events, then rebuild UI each frame.
 */

#include "tapiru/exports.h"
#include "tapiru/widgets/builders.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

    namespace detail {
        class scene;
    }
    using node_id = uint32_t;

    // ── Button ─────────────────────────────────────────────────────────────

    class TAPIRU_API button_builder {
      public:
        explicit button_builder(std::string_view label) : label_(label) {}

        button_builder &on_click(std::function<void()> cb) {
            on_click_ = std::move(cb);
            return *this;
        }
        button_builder &focused(bool f) {
            focused_ = f;
            return *this;
        }
        button_builder &style_override(const style &s) {
            style_ = s;
            return *this;
        }
        button_builder &key(std::string_view k) {
            key_ = detail::fnv1a_hash(k);
            return *this;
        }
        button_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }

        node_id flatten_into(detail::scene &s) const;

        const std::function<void()> &click_handler() const { return on_click_; }

      private:
        std::string label_;
        std::function<void()> on_click_;
        bool focused_ = false;
        style style_;
        uint64_t key_ = 0;
        int16_t z_order_ = 0;
    };

    // ── Checkbox ───────────────────────────────────────────────────────────

    class TAPIRU_API checkbox_builder {
      public:
        checkbox_builder(std::string_view label, bool *value) : label_(label), value_(value) {}

        checkbox_builder &focused(bool f) {
            focused_ = f;
            return *this;
        }
        checkbox_builder &style_override(const style &s) {
            style_ = s;
            return *this;
        }
        checkbox_builder &key(std::string_view k) {
            key_ = detail::fnv1a_hash(k);
            return *this;
        }
        checkbox_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }

        node_id flatten_into(detail::scene &s) const;

        bool *value_ptr() const { return value_; }

      private:
        std::string label_;
        bool *value_ = nullptr;
        bool focused_ = false;
        style style_;
        uint64_t key_ = 0;
        int16_t z_order_ = 0;
    };

    // ── Radio group ────────────────────────────────────────────────────────

    class TAPIRU_API radio_group_builder {
      public:
        radio_group_builder(std::vector<std::string> options, int *selected)
            : options_(std::move(options)), selected_(selected) {}

        radio_group_builder &focused_index(int idx) {
            focused_idx_ = idx;
            return *this;
        }
        radio_group_builder &style_override(const style &s) {
            style_ = s;
            return *this;
        }
        radio_group_builder &key(std::string_view k) {
            key_ = detail::fnv1a_hash(k);
            return *this;
        }
        radio_group_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }

        node_id flatten_into(detail::scene &s) const;

        int *selected_ptr() const { return selected_; }

      private:
        std::vector<std::string> options_;
        int *selected_ = nullptr;
        int focused_idx_ = -1;
        style style_;
        uint64_t key_ = 0;
        int16_t z_order_ = 0;
    };

    // ── Selectable list ────────────────────────────────────────────────────

    class TAPIRU_API selectable_list_builder {
      public:
        selectable_list_builder(std::vector<std::string> items, int *cursor)
            : items_(std::move(items)), cursor_(cursor) {}

        selectable_list_builder &visible_count(uint32_t n) {
            visible_ = n;
            return *this;
        }
        selectable_list_builder &style_override(const style &s) {
            style_ = s;
            return *this;
        }
        selectable_list_builder &highlight_style(const style &s) {
            highlight_ = s;
            return *this;
        }
        selectable_list_builder &key(std::string_view k) {
            key_ = detail::fnv1a_hash(k);
            return *this;
        }
        selectable_list_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }

        node_id flatten_into(detail::scene &s) const;

        int *cursor_ptr() const { return cursor_; }

      private:
        std::vector<std::string> items_;
        int *cursor_ = nullptr;
        uint32_t visible_ = 0; // 0 = show all
        style style_;
        style highlight_;
        uint64_t key_ = 0;
        int16_t z_order_ = 0;
    };

    // ── Text input ─────────────────────────────────────────────────────────

    class TAPIRU_API text_input_builder {
      public:
        explicit text_input_builder(std::string *buffer) : buffer_(buffer) {}

        text_input_builder &placeholder(std::string_view ph) {
            placeholder_ = ph;
            return *this;
        }
        text_input_builder &width(uint32_t w) {
            width_ = w;
            return *this;
        }
        text_input_builder &focused(bool f) {
            focused_ = f;
            return *this;
        }
        text_input_builder &style_override(const style &s) {
            style_ = s;
            return *this;
        }
        text_input_builder &cursor_pos(uint32_t pos) {
            cursor_pos_ = pos;
            return *this;
        }
        text_input_builder &key(std::string_view k) {
            key_ = detail::fnv1a_hash(k);
            return *this;
        }
        text_input_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }

        node_id flatten_into(detail::scene &s) const;

        std::string *buffer_ptr() const { return buffer_; }

      private:
        std::string *buffer_ = nullptr;
        std::string placeholder_;
        uint32_t width_ = 20;
        bool focused_ = false;
        style style_;
        uint32_t cursor_pos_ = 0;
        uint64_t key_ = 0;
        int16_t z_order_ = 0;
    };

    // ── Slider ─────────────────────────────────────────────────────────────

    class TAPIRU_API slider_builder {
      public:
        slider_builder(float *value, float min_val, float max_val) : value_(value), min_(min_val), max_(max_val) {}

        slider_builder &width(uint32_t w) {
            width_ = w;
            return *this;
        }
        slider_builder &focused(bool f) {
            focused_ = f;
            return *this;
        }
        slider_builder &style_override(const style &s) {
            style_ = s;
            return *this;
        }
        slider_builder &show_percentage(bool sp) {
            show_pct_ = sp;
            return *this;
        }
        slider_builder &fill_char(char32_t c) {
            fill_ = c;
            return *this;
        }
        slider_builder &empty_char(char32_t c) {
            empty_ = c;
            return *this;
        }
        slider_builder &key(std::string_view k) {
            key_ = detail::fnv1a_hash(k);
            return *this;
        }
        slider_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }

        node_id flatten_into(detail::scene &s) const;

        float *value_ptr() const { return value_; }

      private:
        float *value_ = nullptr;
        float min_ = 0.0f;
        float max_ = 1.0f;
        uint32_t width_ = 20;
        bool focused_ = false;
        style style_;
        bool show_pct_ = true;
        char32_t fill_ = U'\x2588';  // █
        char32_t empty_ = U'\x2591'; // ░
        uint64_t key_ = 0;
        int16_t z_order_ = 0;
    };

} // namespace tapiru
