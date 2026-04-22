/**
 * @file highlighter.cpp
 * @brief Auto-highlight system implementation.
 */

#include "tapiru/text/highlighter.h"

#include "tapiru/core/ansi.h"

#include <algorithm>
#include <regex>

namespace tapiru {

    // ═══════════════════════════════════════════════════════════════════════
    // regex_highlighter
    // ═══════════════════════════════════════════════════════════════════════

    void regex_highlighter::add_rule(std::string pattern, style sty, int group) {
        rules_.push_back({std::move(pattern), sty, group});
    }

    void regex_highlighter::add_word(std::string_view word, style sty) {
        std::string pat = "\\b";
        pat += word;
        pat += "\\b";
        rules_.push_back({std::move(pat), sty, 0});
    }

    std::vector<highlight_span> regex_highlighter::highlight(std::string_view text) const {
        std::vector<highlight_span> spans;
        spans.reserve(32);

        for (const auto &rule : rules_) {
            try {
                std::regex re(rule.pattern, std::regex::ECMAScript | std::regex::optimize);
                auto begin = std::cregex_iterator(text.data(), text.data() + text.size(), re);
                auto end = std::cregex_iterator();

                for (auto it = begin; it != end; ++it) {
                    const auto &match = *it;
                    int g = rule.group;
                    if (g < 0 || g >= static_cast<int>(match.size())) {
                        g = 0;
                    }
                    if (match[g].matched) {
                        auto off = static_cast<size_t>(match[g].first - text.data());
                        auto len = static_cast<size_t>(match[g].length());
                        if (len > 0) {
                            spans.push_back({off, len, rule.sty, {}});
                        }
                    }
                }
            } catch (const std::regex_error &) {
                // Skip invalid patterns silently
            }
        }

        return spans;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // repr_highlighter — built-in patterns
    // ═══════════════════════════════════════════════════════════════════════

    namespace {

        // Helper: scan all matches of a regex and append spans
        void scan_pattern(std::string_view text, const char *pattern, const style &sty,
                          std::vector<highlight_span> &out, int group = 0) {
            try {
                std::regex re(pattern, std::regex::ECMAScript | std::regex::optimize);
                auto begin = std::cregex_iterator(text.data(), text.data() + text.size(), re);
                auto end = std::cregex_iterator();
                for (auto it = begin; it != end; ++it) {
                    const auto &m = *it;
                    int g = group;
                    if (g < 0 || g >= static_cast<int>(m.size())) {
                        g = 0;
                    }
                    if (m[g].matched) {
                        auto off = static_cast<size_t>(m[g].first - text.data());
                        auto len = static_cast<size_t>(m[g].length());
                        if (len > 0) {
                            out.push_back({off, len, sty, {}});
                        }
                    }
                }
            } catch (const std::regex_error &) {}
        }

        enum class link_kind { url_as_is, file_uri };

        // Helper: scan matches and attach a hyperlink URL to each span
        void scan_pattern_linked(std::string_view text, const char *pattern, const style &sty,
                                 std::vector<highlight_span> &out, link_kind lk, int group = 0) {
            try {
                std::regex re(pattern, std::regex::ECMAScript | std::regex::optimize);
                auto begin = std::cregex_iterator(text.data(), text.data() + text.size(), re);
                auto end = std::cregex_iterator();
                for (auto it = begin; it != end; ++it) {
                    const auto &m = *it;
                    int g = group;
                    if (g < 0 || g >= static_cast<int>(m.size())) {
                        g = 0;
                    }
                    if (m[g].matched) {
                        auto off = static_cast<size_t>(m[g].first - text.data());
                        auto len = static_cast<size_t>(m[g].length());
                        if (len > 0) {
                            std::string matched(text.substr(off, len));
                            std::string link_url;
                            if (lk == link_kind::url_as_is) {
                                link_url = matched;
                            } else {
                                // Convert path to file:// URI
                                link_url = "file:///";
                                for (char c : matched) {
                                    if (c == '\\') {
                                        link_url += '/';
                                    } else {
                                        link_url += c;
                                    }
                                }
                            }
                            out.push_back({off, len, sty, std::move(link_url)});
                        }
                    }
                }
            } catch (const std::regex_error &) {}
        }

    } // anonymous namespace

    repr_highlighter::repr_highlighter() = default;

    repr_highlighter::repr_highlighter(theme t) : theme_(t) {}

    const repr_highlighter &repr_highlighter::instance() {
        static const repr_highlighter inst;
        return inst;
    }

    std::vector<highlight_span> repr_highlighter::highlight(std::string_view text) const {
        std::vector<highlight_span> spans;
        spans.reserve(64);

        // ── String literals ─────────────────────────────────────────────
        // Must come first so quoted content doesn't get matched by other rules.
        // Double-quoted strings (with escape support)
        scan_pattern(text, R"("(?:[^"\\]|\\.)*")", theme_.string, spans);
        // Single-quoted strings (with escape support)
        scan_pattern(text, R"('(?:[^'\\]|\\.)*')", theme_.string, spans);

        // ── URLs ────────────────────────────────────────────────────────
        scan_pattern_linked(text, R"(\b(?:https?|ftp|file|ssh|git)://[^\s<>"'\])\}]+)", theme_.url, spans,
                            link_kind::url_as_is);

        // ── Email addresses ─────────────────────────────────────────────
        scan_pattern(text, R"(\b[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}\b)", theme_.email, spans);

        // ── UUIDs ───────────────────────────────────────────────────────
        scan_pattern(text, R"(\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\b)",
                     theme_.uuid, spans);

        // ── IPv6 addresses (simplified — before IPv4 to avoid partial matches) ──
        scan_pattern(text,
                     R"(\b(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}\b)"
                     R"(|\b(?:[0-9a-fA-F]{1,4}:){1,7}:\b)"
                     R"(|\b::(?:[0-9a-fA-F]{1,4}:){0,5}[0-9a-fA-F]{1,4}\b)"
                     R"(|\b::1\b|\b::\b)",
                     theme_.ipv6, spans);

        // ── IPv4 addresses ──────────────────────────────────────────────
        scan_pattern(text,
                     R"(\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b)",
                     theme_.ipv4, spans);

        // ── ISO 8601 dates/times ────────────────────────────────────────
        scan_pattern(text, R"(\b\d{4}-\d{2}-\d{2}(?:[T ]\d{2}:\d{2}(?::\d{2})?(?:\.\d+)?(?:Z|[+\-]\d{2}:?\d{2})?)?\b)",
                     theme_.iso_date, spans);

        // ── File paths ──────────────────────────────────────────────────
        // Unix paths: /usr/bin/... or ~/...
        // Note: no lookbehinds (MSVC ECMAScript doesn't support them).
        // Use capture group 1 for the actual path.
        scan_pattern_linked(text, R"((?:^|\s)(~?/[\w.\-/]+))", theme_.path, spans, link_kind::file_uri, 1);
        // Windows paths: C:\... or \\server\...
        scan_pattern_linked(text, R"((?:^|\s)([A-Za-z]:\\[\w.\-\\]+|\\\\[\w.\-\\]+))", theme_.path, spans,
                            link_kind::file_uri, 1);

        // ── Environment variables ───────────────────────────────────────
        // $VAR or ${VAR}
        scan_pattern(text, R"(\$\{[a-zA-Z_]\w*\}|\$[a-zA-Z_]\w*)", theme_.env_var, spans);
        // %VAR% (Windows)
        scan_pattern(text, R"(%[a-zA-Z_]\w*%)", theme_.env_var, spans);

        // ── Semantic keywords ───────────────────────────────────────────
        scan_pattern(text, R"(\bTODO\b)", theme_.keyword_todo, spans);
        scan_pattern(text, R"(\bFIXME\b)", theme_.keyword_fixme, spans);
        scan_pattern(text, R"(\bNOTE\b)", theme_.keyword_note, spans);
        scan_pattern(text, R"(\bHACK\b)", theme_.keyword_hack, spans);
        scan_pattern(text, R"(\bBUG\b)", theme_.keyword_bug, spans);
        scan_pattern(text, R"(\bXXX\b)", theme_.keyword_xxx, spans);

        // ── Booleans ────────────────────────────────────────────────────
        scan_pattern(text, R"(\b(?:true|false|True|False|TRUE|FALSE)\b)", theme_.boolean, spans);

        // ── Null values ─────────────────────────────────────────────────
        scan_pattern(text, R"(\b(?:null|nullptr|None|nil|NULL|Null|undefined|NaN|Inf)\b)", theme_.null, spans);

        // ── Numbers ─────────────────────────────────────────────────────
        // Hex: 0x1A2F, 0X1a2f
        scan_pattern(text, R"(\b0[xX][0-9a-fA-F]+(?:_[0-9a-fA-F]+)*\b)", theme_.number_hex, spans);
        // Octal: 0o77, 0O77
        scan_pattern(text, R"(\b0[oO][0-7]+(?:_[0-7]+)*\b)", theme_.number_hex, spans);
        // Binary: 0b1010, 0B1010
        scan_pattern(text, R"(\b0[bB][01]+(?:_[01]+)*\b)", theme_.number_hex, spans);
        // Float with exponent: 3.14e10, 1.5E-3
        scan_pattern(text, R"(\b\d+(?:_\d+)*\.?\d*(?:_\d+)*[eE][+\-]?\d+(?:_\d+)*\b)", theme_.number, spans);
        // Float: 3.14, .5, 1.0f
        scan_pattern(text,
                     R"(\b\d+(?:_\d+)*\.\d+(?:_\d+)*[fFdDlL]?\b)"
                     R"(|\.\d+(?:_\d+)*[fFdDlL]?\b)",
                     theme_.number, spans);
        // Integer: 42, 1_000_000, 42L, 42u
        scan_pattern(text, R"(\b\d+(?:_\d+)*[uUlLzZ]*\b)", theme_.number, spans);

        return spans;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // highlight_chain
    // ═══════════════════════════════════════════════════════════════════════

    void highlight_chain::add(const highlighter &hl) {
        refs_.push_back(&hl);
    }

    void highlight_chain::add(std::shared_ptr<highlighter> hl) {
        refs_.push_back(hl.get());
        owned_.push_back(std::move(hl));
    }

    std::vector<highlight_span> highlight_chain::highlight(std::string_view text) const {
        std::vector<highlight_span> all;
        all.reserve(64);

        for (const auto *hl : refs_) {
            auto spans = hl->highlight(text);
            all.insert(all.end(), spans.begin(), spans.end());
        }

        return all;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Utility functions
    // ═══════════════════════════════════════════════════════════════════════

    std::vector<highlight_span> merge_spans(std::vector<highlight_span> spans, size_t text_length) {
        if (spans.empty()) {
            return {};
        }

        // Sort by offset, then by insertion order (stable sort preserves priority)
        std::stable_sort(spans.begin(), spans.end(),
                         [](const highlight_span &a, const highlight_span &b) { return a.offset < b.offset; });

        // Remove overlaps: earlier spans win
        std::vector<highlight_span> merged;
        merged.reserve(spans.size());

        size_t covered_until = 0;

        for (const auto &s : spans) {
            if (s.offset >= text_length) {
                break;
            }

            size_t start = s.offset;
            size_t end = s.end();
            if (end > text_length) {
                end = text_length;
            }

            if (start < covered_until) {
                // Overlap: trim or skip
                if (end <= covered_until) {
                    continue; // fully covered
                }
                // Partial overlap: trim the start
                start = covered_until;
            }

            if (start < end) {
                merged.push_back({start, end - start, s.sty, s.link});
                covered_until = end;
            }
        }

        return merged;
    }

    std::vector<text_fragment> apply_highlights(std::string_view text, const std::vector<highlight_span> &spans) {
        std::vector<text_fragment> result;
        result.reserve(spans.size() * 2 + 1);

        size_t pos = 0;
        for (const auto &s : spans) {
            // Gap before this span
            if (s.offset > pos) {
                result.push_back({std::string(text.substr(pos, s.offset - pos)), style{}});
            }
            // The highlighted span
            result.push_back({std::string(text.substr(s.offset, s.length)), s.sty});
            pos = s.end();
        }

        // Trailing unstyled text
        if (pos < text.size()) {
            result.push_back({std::string(text.substr(pos)), style{}});
        }

        return result;
    }

    std::vector<text_fragment> highlight_text(std::string_view text, const highlighter &hl) {
        auto spans = hl.highlight(text);
        auto merged = merge_spans(std::move(spans), text.size());
        return apply_highlights(text, merged);
    }

    std::vector<linked_fragment> apply_highlights_linked(std::string_view text,
                                                         const std::vector<highlight_span> &spans) {
        std::vector<linked_fragment> result;
        result.reserve(spans.size() * 2 + 1);

        size_t pos = 0;
        for (const auto &s : spans) {
            if (s.offset > pos) {
                result.push_back({std::string(text.substr(pos, s.offset - pos)), style{}, {}});
            }
            result.push_back({std::string(text.substr(s.offset, s.length)), s.sty, s.link});
            pos = s.end();
        }

        if (pos < text.size()) {
            result.push_back({std::string(text.substr(pos)), style{}, {}});
        }

        return result;
    }

    std::vector<linked_fragment> highlight_text_linked(std::string_view text, const highlighter &hl) {
        auto spans = hl.highlight(text);
        auto merged = merge_spans(std::move(spans), text.size());
        return apply_highlights_linked(text, merged);
    }

} // namespace tapiru
