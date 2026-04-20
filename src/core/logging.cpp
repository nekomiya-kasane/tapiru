/**
 * @file logging.cpp
 * @brief Rich logging handler implementation.
 */

#include "tapiru/core/logging.h"

#include "detail/scene.h"
#include "detail/widget_types.h"
#include "tapiru/core/console.h"
#include "tapiru/widgets/builders.h"

#include <algorithm>
#include <ctime>
#include <string>

namespace tapiru {

// ── Log level helpers ───────────────────────────────────────────────────

std::string_view log_level_name(log_level lv) noexcept {
    switch (lv) {
    case log_level::trace:
        return "TRACE";
    case log_level::debug:
        return "DEBUG";
    case log_level::info:
        return "INFO";
    case log_level::warn:
        return "WARN";
    case log_level::error:
        return "ERROR";
    case log_level::fatal:
        return "FATAL";
    }
    return "?????";
}

std::string_view log_level_style(log_level lv) noexcept {
    switch (lv) {
    case log_level::trace:
        return "dim";
    case log_level::debug:
        return "cyan";
    case log_level::info:
        return "green";
    case log_level::warn:
        return "bold yellow";
    case log_level::error:
        return "bold red";
    case log_level::fatal:
        return "bold white on_red";
    }
    return "";
}

// ── Log handler ─────────────────────────────────────────────────────────

log_handler::log_handler(console &con) : console_(con) {}

void log_handler::log(log_level lv, std::string_view message) {
    log_record rec;
    rec.level = lv;
    rec.message = std::string(message);
    rec.module = module_;
    rec.tag = tag_;
    rec.timestamp = std::chrono::system_clock::now();
    log(rec);
}

void log_handler::log(const log_record &record) {
    if (record.level < min_level_) return;
    if (!passes_filter(record)) return;

    std::string markup;
    markup.reserve(128);

    // Timestamp
    if (show_time_) {
        auto tt = std::chrono::system_clock::to_time_t(record.timestamp);
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &tt);
#else
        localtime_r(&tt, &tm_buf);
#endif
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_buf);
        markup += "[dim]";
        markup += time_str;
        markup += "[/dim] ";
    }

    // Level
    if (show_level_) {
        auto style_tag = log_level_style(record.level);
        auto level_name = log_level_name(record.level);

        markup += '[';
        markup += style_tag;
        markup += ']';

        // Pad level name to 5 chars
        markup += level_name;
        for (size_t i = level_name.size(); i < 5; ++i) markup += ' ';

        markup += "[/] ";
    }

    // Module/tag prefix
    if (!record.module.empty()) {
        markup += "[dim]";
        markup += record.module;
        markup += "[/dim] ";
    }

    // Message
    markup += record.message;

    // Structured fields
    if (!record.fields.empty()) {
        markup += " [dim]";
        bool first = true;
        for (const auto &[k, v] : record.fields) {
            if (!first) markup += ", ";
            markup += k;
            markup += '=';
            markup += v;
            first = false;
        }
        markup += "[/dim]";
    }

    console_.print(markup);
}

void log_handler::log_structured(log_level lv, std::string_view message,
                                 std::unordered_map<std::string, std::string> fields) {
    log_record rec;
    rec.level = lv;
    rec.message = std::string(message);
    rec.module = module_;
    rec.tag = tag_;
    rec.fields = std::move(fields);
    rec.timestamp = std::chrono::system_clock::now();
    log(rec);
}

bool log_handler::passes_filter(const log_record &r) const {
    if (!allowed_modules_.empty() && !r.module.empty()) {
        if (allowed_modules_.find(r.module) == allowed_modules_.end()) return false;
    }
    if (!allowed_tags_.empty() && !r.tag.empty()) {
        if (allowed_tags_.find(r.tag) == allowed_tags_.end()) return false;
    }
    return true;
}

// ── log_panel_builder ─────────────────────────────────────────────────

void log_panel_builder::push(const log_record &record) {
    std::string line;
    auto style_tag = log_level_style(record.level);
    auto level_name = log_level_name(record.level);

    line += '[';
    line += style_tag;
    line += ']';
    line += level_name;
    for (size_t i = level_name.size(); i < 5; ++i) line += ' ';
    line += "[/] ";
    line += record.message;

    if (!record.fields.empty()) {
        line += " [dim]";
        bool first = true;
        for (const auto &[k, v] : record.fields) {
            if (!first) line += ", ";
            line += k;
            line += '=';
            line += v;
            first = false;
        }
        line += "[/dim]";
    }

    lines_.push_back(std::move(line));
    if (lines_.size() > max_lines_) {
        lines_.erase(lines_.begin());
    }
}

void log_panel_builder::clear() {
    lines_.clear();
}

node_id log_panel_builder::flatten_into(detail::scene &s) const {
    if (lines_.empty()) {
        return text_builder("(no logs)").flatten_into(s);
    }

    uint32_t start = 0;
    uint32_t count = static_cast<uint32_t>(lines_.size());
    if (count > visible_) {
        if (auto_scroll_) {
            start = count - visible_;
        }
        count = visible_;
    }

    std::string markup;
    for (uint32_t i = 0; i < count; ++i) {
        if (i > 0) markup += '\n';
        markup += lines_[start + i];
    }

    return text_builder(markup).flatten_into(s);
}

} // namespace tapiru
