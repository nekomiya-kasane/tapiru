#pragma once

/**
 * @file log_viewer.h
 * @brief Log viewer widget — scrollable, filterable log display.
 */

#include "tapiru/core/logging.h"
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

    struct log_entry {
        log_level level;
        std::string timestamp;
        std::string message;
    };

    class TAPIRU_API log_viewer_builder {
      public:
        log_viewer_builder() = default;

        log_viewer_builder &entries(const std::vector<log_entry> *e) {
            entries_ = e;
            return *this;
        }
        log_viewer_builder &scroll(const int *s) {
            scroll_ = s;
            return *this;
        }
        log_viewer_builder &min_level(log_level l) {
            min_level_ = l;
            return *this;
        }
        log_viewer_builder &filter(const std::string *f) {
            filter_ = f;
            return *this;
        }
        log_viewer_builder &height(uint32_t h) {
            height_ = h;
            return *this;
        }
        log_viewer_builder &show_timestamp(bool v = true) {
            show_ts_ = v;
            return *this;
        }
        log_viewer_builder &show_level(bool v = true) {
            show_lvl_ = v;
            return *this;
        }
        log_viewer_builder &border(border_style bs) {
            border_ = bs;
            return *this;
        }
        log_viewer_builder &z_order(int16_t z) {
            z_order_ = z;
            return *this;
        }
        log_viewer_builder &key(std::string_view k);

        node_id flatten_into(detail::scene &s) const;

      private:
        const std::vector<log_entry> *entries_ = nullptr;
        const int *scroll_ = nullptr;
        const std::string *filter_ = nullptr;
        log_level min_level_ = log_level::debug;
        uint32_t height_ = 10;
        bool show_ts_ = true;
        bool show_lvl_ = true;
        border_style border_ = border_style::rounded;
        int16_t z_order_ = 0;
        uint64_t key_ = 0;
    };

} // namespace tapiru
