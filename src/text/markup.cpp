/**
 * @file markup.cpp
 * @brief Rich-like markup parser implementation.
 */

#include "tapiru/text/markup.h"

#include <algorithm>
#include <charconv>
#include <cstring>

namespace tapiru {

namespace {

// ── Tag → style mapping ─────────────────────────────────────────────────

struct tag_entry {
    const char* name;
    enum class kind : uint8_t { fg, bg, attr } type;
    union {
        color   col;
        attr    at;
    };
};

constexpr color make_fg(uint8_t idx) { return color::from_index_16(idx); }

// Tag lookup table (sorted for potential binary search, but linear is fine for ~40 entries)
bool try_parse_hex_color(std::string_view s, color& out) {
    if (s.size() != 7 || s[0] != '#') return false;
    uint8_t r, g, b;
    auto parse_byte = [](const char* p, uint8_t& v) -> bool {
        auto [ptr, ec] = std::from_chars(p, p + 2, v, 16);
        return ec == std::errc{};
    };
    if (!parse_byte(s.data() + 1, r)) return false;
    if (!parse_byte(s.data() + 3, g)) return false;
    if (!parse_byte(s.data() + 5, b)) return false;
    out = color::from_rgb(r, g, b);
    return true;
}

bool try_parse_rgb_func(std::string_view tag, color& out, bool is_bg) {
    // "rgb(R,G,B)" or "on_rgb(R,G,B)"
    auto prefix = is_bg ? std::string_view("on_rgb(") : std::string_view("rgb(");
    if (!tag.starts_with(prefix) || tag.back() != ')') return false;
    auto inner = tag.substr(prefix.size(), tag.size() - prefix.size() - 1);
    // Parse 3 comma-separated values
    auto c1 = inner.find(',');
    if (c1 == std::string_view::npos) return false;
    auto c2 = inner.find(',', c1 + 1);
    if (c2 == std::string_view::npos) return false;
    uint8_t r = 0, g = 0, b = 0;
    auto pr = std::from_chars(inner.data(), inner.data() + c1, r);
    if (pr.ec != std::errc{}) return false;
    auto pg = std::from_chars(inner.data() + c1 + 1, inner.data() + c2, g);
    if (pg.ec != std::errc{}) return false;
    auto pb = std::from_chars(inner.data() + c2 + 1, inner.data() + inner.size(), b);
    if (pb.ec != std::errc{}) return false;
    out = color::from_rgb(r, g, b);
    return true;
}

bool try_parse_color256(std::string_view tag, color& out, bool is_bg) {
    auto prefix = is_bg ? std::string_view("on_color256(") : std::string_view("color256(");
    if (!tag.starts_with(prefix) || tag.back() != ')') return false;
    auto inner = tag.substr(prefix.size(), tag.size() - prefix.size() - 1);
    uint8_t idx = 0;
    auto res = std::from_chars(inner.data(), inner.data() + inner.size(), idx);
    if (res.ec != std::errc{}) return false;
    out = color::from_index_256(idx);
    return true;
}

bool apply_tag(std::string_view tag, style& s) {
    // Attributes
    if (tag == "bold")             { s.attrs |= attr::bold;             return true; }
    if (tag == "dim")              { s.attrs |= attr::dim;              return true; }
    if (tag == "italic")           { s.attrs |= attr::italic;           return true; }
    if (tag == "underline")        { s.attrs |= attr::underline;        return true; }
    if (tag == "blink")            { s.attrs |= attr::blink;            return true; }
    if (tag == "reverse")          { s.attrs |= attr::reverse;          return true; }
    if (tag == "hidden")           { s.attrs |= attr::hidden;           return true; }
    if (tag == "strike")           { s.attrs |= attr::strike;           return true; }
    if (tag == "double_underline") { s.attrs |= attr::double_underline; return true; }
    if (tag == "overline")         { s.attrs |= attr::overline;         return true; }
    if (tag == "superscript")      { s.attrs |= attr::superscript;      return true; }
    if (tag == "subscript")        { s.attrs |= attr::subscript;        return true; }

    // Negation tags
    if (tag == "not_bold")      { s.attrs = s.attrs & ~attr::bold;      return true; }
    if (tag == "not_dim")       { s.attrs = s.attrs & ~attr::dim;       return true; }
    if (tag == "not_italic")    { s.attrs = s.attrs & ~attr::italic;    return true; }
    if (tag == "not_underline") { s.attrs = s.attrs & ~attr::underline; return true; }
    if (tag == "not_blink")     { s.attrs = s.attrs & ~attr::blink;     return true; }
    if (tag == "not_reverse")   { s.attrs = s.attrs & ~attr::reverse;   return true; }
    if (tag == "not_hidden")    { s.attrs = s.attrs & ~attr::hidden;    return true; }
    if (tag == "not_strike")    { s.attrs = s.attrs & ~attr::strike;    return true; }
    if (tag == "not_overline")  { s.attrs = s.attrs & ~attr::overline;  return true; }

    // Color reset
    if (tag == "default")    { s.fg = color::default_color(); return true; }
    if (tag == "on_default") { s.bg = color::default_color(); return true; }

    // Semantic aliases
    if (tag == "error")        { s.attrs |= attr::bold; s.fg = colors::red;    return true; }
    if (tag == "warning")      { s.attrs |= attr::bold; s.fg = colors::yellow; return true; }
    if (tag == "success")      { s.attrs |= attr::bold; s.fg = colors::green;  return true; }
    if (tag == "info")         { s.attrs |= attr::bold; s.fg = colors::cyan;   return true; }
    if (tag == "debug")        { s.attrs |= attr::dim; return true; }
    if (tag == "muted")        { s.attrs |= attr::dim; return true; }
    if (tag == "highlight")    { s.attrs |= attr::bold; s.fg = colors::black; s.bg = colors::yellow; return true; }
    if (tag == "code_inline")  { s.attrs |= attr::bold; s.fg = colors::cyan; s.bg = color::from_rgb(0x1e, 0x1e, 0x2e); return true; }
    if (tag == "url")          { s.attrs |= attr::underline; s.fg = colors::bright_blue; return true; }
    if (tag == "path")         { s.attrs |= attr::italic; s.fg = colors::bright_green; return true; }
    if (tag == "key")          { s.attrs |= attr::bold; s.fg = colors::white; s.bg = color::from_rgb(0x3a, 0x3a, 0x4a); return true; }
    if (tag == "badge.red")    { s.attrs |= attr::bold; s.fg = colors::white; s.bg = colors::red;    return true; }
    if (tag == "badge.green")  { s.attrs |= attr::bold; s.fg = colors::white; s.bg = colors::green;  return true; }
    if (tag == "badge.blue")   { s.attrs |= attr::bold; s.fg = colors::white; s.bg = colors::blue;   return true; }
    if (tag == "badge.yellow") { s.attrs |= attr::bold; s.fg = colors::black; s.bg = colors::yellow; return true; }

    // Foreground colors
    if (tag == "black")          { s.fg = colors::black;          return true; }
    if (tag == "red")            { s.fg = colors::red;            return true; }
    if (tag == "green")          { s.fg = colors::green;          return true; }
    if (tag == "yellow")         { s.fg = colors::yellow;         return true; }
    if (tag == "blue")           { s.fg = colors::blue;           return true; }
    if (tag == "magenta")        { s.fg = colors::magenta;        return true; }
    if (tag == "cyan")           { s.fg = colors::cyan;           return true; }
    if (tag == "white")          { s.fg = colors::white;          return true; }
    if (tag == "bright_black")   { s.fg = colors::bright_black;   return true; }
    if (tag == "bright_red")     { s.fg = colors::bright_red;     return true; }
    if (tag == "bright_green")   { s.fg = colors::bright_green;   return true; }
    if (tag == "bright_yellow")  { s.fg = colors::bright_yellow;  return true; }
    if (tag == "bright_blue")    { s.fg = colors::bright_blue;    return true; }
    if (tag == "bright_magenta") { s.fg = colors::bright_magenta; return true; }
    if (tag == "bright_cyan")    { s.fg = colors::bright_cyan;    return true; }
    if (tag == "bright_white")   { s.fg = colors::bright_white;   return true; }

    // Background colors (on_ prefix)
    if (tag == "on_black")          { s.bg = colors::black;          return true; }
    if (tag == "on_red")            { s.bg = colors::red;            return true; }
    if (tag == "on_green")          { s.bg = colors::green;          return true; }
    if (tag == "on_yellow")         { s.bg = colors::yellow;         return true; }
    if (tag == "on_blue")           { s.bg = colors::blue;           return true; }
    if (tag == "on_magenta")        { s.bg = colors::magenta;        return true; }
    if (tag == "on_cyan")           { s.bg = colors::cyan;           return true; }
    if (tag == "on_white")          { s.bg = colors::white;          return true; }
    if (tag == "on_bright_black")   { s.bg = colors::bright_black;   return true; }
    if (tag == "on_bright_red")     { s.bg = colors::bright_red;     return true; }
    if (tag == "on_bright_green")   { s.bg = colors::bright_green;   return true; }
    if (tag == "on_bright_yellow")  { s.bg = colors::bright_yellow;  return true; }
    if (tag == "on_bright_blue")    { s.bg = colors::bright_blue;    return true; }
    if (tag == "on_bright_magenta") { s.bg = colors::bright_magenta; return true; }
    if (tag == "on_bright_cyan")    { s.bg = colors::bright_cyan;    return true; }
    if (tag == "on_bright_white")   { s.bg = colors::bright_white;   return true; }

    // RGB hex: #RRGGBB (fg)
    if (tag.size() == 7 && tag[0] == '#') {
        color c;
        if (try_parse_hex_color(tag, c)) {
            s.fg = c;
            return true;
        }
    }

    // RGB hex: on_#RRGGBB (bg)
    if (tag.size() == 10 && tag.substr(0, 3) == "on_" && tag[3] == '#') {
        color c;
        if (try_parse_hex_color(tag.substr(3), c)) {
            s.bg = c;
            return true;
        }
    }

    // rgb(R,G,B) / on_rgb(R,G,B)
    if (tag.starts_with("rgb(")) {
        color c;
        if (try_parse_rgb_func(tag, c, false)) { s.fg = c; return true; }
    }
    if (tag.starts_with("on_rgb(")) {
        color c;
        if (try_parse_rgb_func(tag, c, true)) { s.bg = c; return true; }
    }

    // color256(N) / on_color256(N)
    if (tag.starts_with("color256(")) {
        color c;
        if (try_parse_color256(tag, c, false)) { s.fg = c; return true; }
    }
    if (tag.starts_with("on_color256(")) {
        color c;
        if (try_parse_color256(tag, c, true)) { s.bg = c; return true; }
    }

    return false;  // unknown tag
}

void apply_compound_tag(std::string_view tag_content, style& s) {
    // Split by spaces and apply each sub-tag
    size_t pos = 0;
    while (pos < tag_content.size()) {
        // Skip whitespace
        while (pos < tag_content.size() && tag_content[pos] == ' ') ++pos;
        if (pos >= tag_content.size()) break;

        // Find end of token
        size_t end = tag_content.find(' ', pos);
        if (end == std::string_view::npos) end = tag_content.size();

        auto token = tag_content.substr(pos, end - pos);
        if (!token.empty()) {
            apply_tag(token, s);
        }
        pos = end;
    }
}

}  // anonymous namespace

// ── parse_markup ────────────────────────────────────────────────────────

std::vector<text_fragment> parse_markup(std::string_view markup) {
    std::vector<text_fragment> result;
    std::vector<style> style_stack;
    style_stack.push_back(style{});  // default style

    std::string current_text;
    size_t i = 0;

    while (i < markup.size()) {
        if (markup[i] == '[') {
            // Check for escaped [[
            if (i + 1 < markup.size() && markup[i + 1] == '[') {
                current_text += '[';
                i += 2;
                continue;
            }

            // Find closing ]
            size_t close = markup.find(']', i + 1);
            if (close == std::string_view::npos) {
                // No closing bracket — treat as literal
                current_text += markup[i];
                ++i;
                continue;
            }

            // Flush current text
            if (!current_text.empty()) {
                result.push_back({std::move(current_text), style_stack.back()});
                current_text.clear();
            }

            auto tag_content = markup.substr(i + 1, close - i - 1);

            if (tag_content == "/") {
                // [/] — reset to default (pop all)
                style_stack.clear();
                style_stack.push_back(style{});
            } else if (tag_content.size() > 0 && tag_content[0] == '/') {
                // [/tag] — closing tag: pop one level
                if (style_stack.size() > 1) {
                    style_stack.pop_back();
                }
            } else {
                // [tag] — opening tag: push new style based on current
                style new_style = style_stack.back();
                apply_compound_tag(tag_content, new_style);
                style_stack.push_back(new_style);
            }

            i = close + 1;
        } else {
            current_text += markup[i];
            ++i;
        }
    }

    // Flush remaining text
    if (!current_text.empty()) {
        result.push_back({std::move(current_text), style_stack.back()});
    }

    return result;
}

// ── strip_markup ────────────────────────────────────────────────────────

std::string strip_markup(std::string_view markup) {
    std::string result;
    result.reserve(markup.size());

    size_t i = 0;
    while (i < markup.size()) {
        if (markup[i] == '[') {
            // Escaped [[
            if (i + 1 < markup.size() && markup[i + 1] == '[') {
                result += '[';
                i += 2;
                continue;
            }

            // Skip tag
            size_t close = markup.find(']', i + 1);
            if (close == std::string_view::npos) {
                result += markup[i];
                ++i;
            } else {
                i = close + 1;
            }
        } else {
            result += markup[i];
            ++i;
        }
    }

    return result;
}

}  // namespace tapiru
