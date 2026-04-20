#pragma once

/**
 * @file logging.h
 * @brief Rich logging handler: structured log → styled console output.
 *
 * Usage:
 *   tapiru::console con;
 *   tapiru::log_handler logger(con);
 *   logger.info("Server started on port {}", 8080);
 *   logger.error("Connection failed: {}", err_msg);
 *
 * Or with custom format:
 *   logger.set_format("[{level}] {time} - {message}");
 */

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tapiru/exports.h"

namespace tapiru {

class console;

// ── Log levels ──────────────────────────────────────────────────────────

enum class log_level : uint8_t {
    trace = 0,
    debug = 1,
    info  = 2,
    warn  = 3,
    error = 4,
    fatal = 5,
};

/** @brief Convert log level to string. */
[[nodiscard]] TAPIRU_API std::string_view log_level_name(log_level lv) noexcept;

/** @brief Get the default markup tag for a log level. */
[[nodiscard]] TAPIRU_API std::string_view log_level_style(log_level lv) noexcept;

// ── Log record ──────────────────────────────────────────────────────────

struct log_record {
    log_level   level = log_level::info;
    std::string message;
    std::string logger_name;
    std::string module;
    std::string tag;
    std::unordered_map<std::string, std::string> fields;
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
};

// ── Log handler ─────────────────────────────────────────────────────────

/**
 * @brief Rich logging handler that formats log records with markup and
 * sends them to a console.
 */
class TAPIRU_API log_handler {
public:
    explicit log_handler(console& con);

    /** @brief Set minimum log level. Messages below this are suppressed. */
    void set_level(log_level min_level) noexcept { min_level_ = min_level; }

    /** @brief Get current minimum log level. */
    [[nodiscard]] log_level level() const noexcept { return min_level_; }

    /** @brief Log a pre-built record. */
    void log(const log_record& record);

    /** @brief Log a message at a specific level. */
    void log(log_level lv, std::string_view message);

    /** @brief Convenience methods for each level. */
    void trace(std::string_view msg) { log(log_level::trace, msg); }
    void debug(std::string_view msg) { log(log_level::debug, msg); }
    void info(std::string_view msg)  { log(log_level::info,  msg); }
    void warn(std::string_view msg)  { log(log_level::warn,  msg); }
    void error(std::string_view msg) { log(log_level::error, msg); }
    void fatal(std::string_view msg) { log(log_level::fatal, msg); }

    /** @brief Enable/disable timestamp in output. */
    void show_timestamp(bool v) noexcept { show_time_ = v; }

    /** @brief Enable/disable log level label in output. */
    void show_level(bool v) noexcept { show_level_ = v; }

    /** @brief Set module name for subsequent log calls. */
    void set_module(std::string mod) { module_ = std::move(mod); }

    /** @brief Set tag for subsequent log calls. */
    void set_tag(std::string t) { tag_ = std::move(t); }

    /** @brief Add a module filter. Only records matching allowed modules pass. */
    void allow_module(std::string mod) { allowed_modules_.insert(std::move(mod)); }

    /** @brief Add a tag filter. Only records matching allowed tags pass. */
    void allow_tag(std::string t) { allowed_tags_.insert(std::move(t)); }

    /** @brief Clear all module/tag filters (allow everything). */
    void clear_filters() { allowed_modules_.clear(); allowed_tags_.clear(); }

    /** @brief Log with structured fields. */
    void log_structured(log_level lv, std::string_view message,
                        std::unordered_map<std::string, std::string> fields);

private:
    bool passes_filter(const log_record& r) const;

    console&  console_;
    log_level min_level_ = log_level::trace;
    bool      show_time_  = true;
    bool      show_level_ = true;
    std::string module_;
    std::string tag_;
    std::unordered_set<std::string> allowed_modules_;
    std::unordered_set<std::string> allowed_tags_;
};

// ── Log panel builder ──────────────────────────────────────────────────

namespace detail { class scene; }
using node_id = uint32_t;

/**
 * @brief A widget that displays a scrolling log buffer.
 *
 * Stores recent log records and renders them as styled text.
 */
class TAPIRU_API log_panel_builder {
public:
    explicit log_panel_builder(uint32_t max_lines = 100)
        : max_lines_(max_lines) {}

    void push(const log_record& record);
    void clear();

    [[nodiscard]] size_t size() const noexcept { return lines_.size(); }
    [[nodiscard]] uint32_t max_lines() const noexcept { return max_lines_; }
    [[nodiscard]] const std::vector<std::string>& lines() const noexcept { return lines_; }

    log_panel_builder& visible_lines(uint32_t n) { visible_ = n; return *this; }
    log_panel_builder& scroll_to_bottom(bool v = true) { auto_scroll_ = v; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    uint32_t max_lines_ = 100;
    uint32_t visible_ = 20;
    bool auto_scroll_ = true;
    std::vector<std::string> lines_;  // pre-formatted markup lines
};

}  // namespace tapiru
