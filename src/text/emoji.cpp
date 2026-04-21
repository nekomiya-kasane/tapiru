/**
 * @file emoji.cpp
 * @brief Emoji shortcode lookup table and replacement.
 */

#include "tapiru/text/emoji.h"

#include <algorithm>
#include <cstring>

namespace tapiru {

namespace {

struct emoji_entry {
    const char *shortcode; // without colons
    char32_t codepoint;
};

// Sorted by shortcode for binary search
constexpr emoji_entry g_emoji_table[] = {
    {"100", U'\U0001F4AF'},
    {"alien", U'\U0001F47D'},
    {"anger", U'\U0001F4A2'},
    {"apple", U'\U0001F34E'},
    {"art", U'\U0001F3A8'},
    {"beer", U'\U0001F37A'},
    {"bell", U'\U0001F514'},
    {"bomb", U'\U0001F4A3'},
    {"book", U'\U0001F4D6'},
    {"boom", U'\U0001F4A5'},
    {"bug", U'\U0001F41B'},
    {"bulb", U'\U0001F4A1'},
    {"calendar", U'\U0001F4C5'},
    {"camera", U'\U0001F4F7'},
    {"cat", U'\U0001F431'},
    {"check", U'\x2705'},
    {"checkmark", U'\x2714'},
    {"clap", U'\U0001F44F'},
    {"clock", U'\U0001F550'},
    {"cloud", U'\x2601'},
    {"coffee", U'\x2615'},
    {"construction", U'\U0001F6A7'},
    {"cool", U'\U0001F60E'},
    {"cross", U'\x274C'},
    {"crown", U'\U0001F451'},
    {"cry", U'\U0001F622'},
    {"diamond", U'\U0001F48E'},
    {"dog", U'\U0001F436'},
    {"earth", U'\U0001F30D'},
    {"email", U'\U0001F4E7'},
    {"exclamation", U'\x2757'},
    {"eyes", U'\U0001F440'},
    {"fire", U'\U0001F525'},
    {"flag", U'\U0001F3C1'},
    {"flower", U'\U0001F33B'},
    {"folder", U'\U0001F4C1'},
    {"gear", U'\x2699'},
    {"ghost", U'\U0001F47B'},
    {"gift", U'\U0001F381'},
    {"globe", U'\U0001F310'},
    {"green_heart", U'\U0001F49A'},
    {"grin", U'\U0001F601'},
    {"hammer", U'\U0001F528'},
    {"heart", U'\x2764'},
    {"heavy_check_mark", U'\x2714'},
    {"hourglass", U'\x231B'},
    {"house", U'\U0001F3E0'},
    {"info", U'\x2139'},
    {"key", U'\U0001F511'},
    {"laugh", U'\U0001F602'},
    {"lightning", U'\x26A1'},
    {"link", U'\U0001F517'},
    {"lock", U'\U0001F512'},
    {"mag", U'\U0001F50D'},
    {"memo", U'\U0001F4DD'},
    {"money", U'\U0001F4B0'},
    {"moon", U'\U0001F319'},
    {"muscle", U'\U0001F4AA'},
    {"music", U'\U0001F3B5'},
    {"ok", U'\U0001F44C'},
    {"package", U'\U0001F4E6'},
    {"party", U'\U0001F389'},
    {"pencil", U'\x270F'},
    {"phone", U'\U0001F4F1'},
    {"pin", U'\U0001F4CC'},
    {"pizza", U'\U0001F355'},
    {"point_right", U'\U0001F449'},
    {"poop", U'\U0001F4A9'},
    {"pray", U'\U0001F64F'},
    {"question", U'\x2753'},
    {"rainbow", U'\U0001F308'},
    {"recycle", U'\x267B'},
    {"rocket", U'\U0001F680'},
    {"rose", U'\U0001F339'},
    {"sad", U'\U0001F61E'},
    {"scissors", U'\x2702'},
    {"skull", U'\U0001F480'},
    {"smile", U'\U0001F604'},
    {"snake", U'\U0001F40D'},
    {"snowflake", U'\x2744'},
    {"sparkles", U'\x2728'},
    {"star", U'\x2B50'},
    {"stop", U'\U0001F6D1'},
    {"sun", U'\x2600'},
    {"tada", U'\U0001F389'},
    {"target", U'\U0001F3AF'},
    {"thinking", U'\U0001F914'},
    {"thumbsdown", U'\U0001F44E'},
    {"thumbsup", U'\U0001F44D'},
    {"timer", U'\x23F1'},
    {"tools", U'\U0001F6E0'},
    {"tree", U'\U0001F333'},
    {"trophy", U'\U0001F3C6'},
    {"truck", U'\U0001F69A'},
    {"turtle", U'\U0001F422'},
    {"umbrella", U'\x2602'},
    {"unlock", U'\U0001F513'},
    {"warning", U'\x26A0'},
    {"wave", U'\U0001F44B'},
    {"wrench", U'\U0001F527'},
    {"x", U'\x274C'},
    {"zap", U'\x26A1'},
};

constexpr size_t g_emoji_count = sizeof(g_emoji_table) / sizeof(g_emoji_table[0]);

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

} // anonymous namespace

char32_t emoji_lookup(std::string_view shortcode) noexcept {
    // Strip colons if present
    if (shortcode.size() >= 2 && shortcode.front() == ':' && shortcode.back() == ':') {
        shortcode = shortcode.substr(1, shortcode.size() - 2);
    }
    if (shortcode.empty()) {
        return 0;
    }

    // Binary search
    size_t lo = 0, hi = g_emoji_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int cmp = shortcode.compare(g_emoji_table[mid].shortcode);
        if (cmp == 0) {
            return g_emoji_table[mid].codepoint;
        }
        if (cmp < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return 0;
}

std::string replace_emoji(std::string_view input) {
    std::string result;
    result.reserve(input.size());

    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == ':') {
            // Find closing colon
            size_t close = input.find(':', i + 1);
            if (close != std::string_view::npos && close > i + 1) {
                auto code = input.substr(i, close - i + 1);
                char32_t cp = emoji_lookup(code);
                if (cp != 0) {
                    append_utf8(result, cp);
                    i = close + 1;
                    continue;
                }
            }
        }
        result += input[i];
        ++i;
    }

    return result;
}

} // namespace tapiru
