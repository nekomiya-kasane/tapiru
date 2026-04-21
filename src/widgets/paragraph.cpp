/**
 * @file paragraph.cpp
 * @brief Word-wrapping paragraph element factory implementations.
 */

#include "tapiru/widgets/paragraph.h"

#include "tapiru/widgets/builders.h"

#include <string>
#include <vector>

namespace tapiru {

namespace {

// Split text into words (space-delimited), preserving newlines as empty markers
std::vector<std::string> split_words(std::string_view text) {
    std::vector<std::string> words;
    std::string current;
    for (char c : text) {
        if (c == '\n') {
            if (!current.empty()) {
                words.push_back(std::move(current));
                current.clear();
            }
            words.emplace_back("\n");
        } else if (c == ' ' || c == '\t') {
            if (!current.empty()) {
                words.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        words.push_back(std::move(current));
    }
    return words;
}

// Word-wrap into lines of max_width characters
std::vector<std::string> wrap_lines(const std::vector<std::string> &words, uint32_t max_width) {
    std::vector<std::string> lines;
    std::string line;
    uint32_t line_len = 0;

    for (const auto &word : words) {
        if (word == "\n") {
            lines.push_back(std::move(line));
            line.clear();
            line_len = 0;
            continue;
        }
        auto wlen = static_cast<uint32_t>(word.size());
        if (line_len == 0) {
            // First word on line — force it even if too long
            line = word;
            line_len = wlen;
        } else if (line_len + 1 + wlen <= max_width) {
            line += ' ';
            line += word;
            line_len += 1 + wlen;
        } else {
            // Start new line
            lines.push_back(std::move(line));
            line = word;
            line_len = wlen;
        }
    }
    if (!line.empty() || lines.empty()) {
        lines.push_back(std::move(line));
    }
    return lines;
}

// Justify a line to exactly target_width by inserting extra spaces between words
std::string justify_line(const std::string &line, uint32_t target_width) {
    auto words = split_words(line);
    if (words.size() <= 1) {
        return line;
    }

    uint32_t total_word_len = 0;
    for (const auto &w : words) {
        total_word_len += static_cast<uint32_t>(w.size());
    }

    uint32_t total_spaces = target_width > total_word_len ? target_width - total_word_len : 0;
    uint32_t gaps = static_cast<uint32_t>(words.size()) - 1;
    if (gaps == 0) {
        return line;
    }

    uint32_t base_spaces = total_spaces / gaps;
    uint32_t extra = total_spaces % gaps;

    std::string result;
    for (size_t i = 0; i < words.size(); ++i) {
        result += words[i];
        if (i < words.size() - 1) {
            uint32_t sp = base_spaces + (i < extra ? 1 : 0);
            result.append(sp, ' ');
        }
    }
    return result;
}

} // anonymous namespace

element make_paragraph(std::string_view text) {
    // Use a default width; the actual wrapping happens at a fixed width
    // since we don't know the container width at element creation time.
    // We'll wrap at 80 columns as a reasonable default.
    constexpr uint32_t default_width = 80;
    auto words = split_words(text);
    auto lines = wrap_lines(words, default_width);

    auto rb = rows_builder();
    for (const auto &line : lines) {
        rb.add(text_builder(line));
    }
    return element(std::move(rb));
}

element make_paragraph_justify(std::string_view text) {
    constexpr uint32_t default_width = 80;
    auto words = split_words(text);
    auto lines = wrap_lines(words, default_width);

    auto rb = rows_builder();
    for (size_t i = 0; i < lines.size(); ++i) {
        // Don't justify the last line
        if (i < lines.size() - 1 && !lines[i].empty()) {
            auto justified = justify_line(lines[i], default_width);
            rb.add(text_builder(justified));
        } else {
            rb.add(text_builder(lines[i]));
        }
    }
    return element(std::move(rb));
}

} // namespace tapiru
