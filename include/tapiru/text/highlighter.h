#pragma once

/**
 * @file highlighter.h
 * @brief Auto-highlight system for terminal output.
 *
 * Automatically detects and styles patterns in plain text:
 *   - Numbers (int, float, hex, octal, binary, scientific)
 *   - String literals ("...", '...')
 *   - URLs (http://, https://, ftp://)
 *   - File paths (/unix/path, C:\windows\path)
 *   - UUIDs (550e8400-e29b-41d4-a716-446655440000)
 *   - IP addresses (IPv4 and IPv6)
 *   - Booleans and null (true, false, null, nullptr, None, nil)
 *   - Environment variables ($VAR, %VAR%)
 *   - Email addresses
 *   - ISO 8601 dates/times
 *   - Semantic keywords (TODO, FIXME, HACK, NOTE, BUG, XXX)
 *
 * Usage:
 *   tapiru::console con;
 *   con.set_highlighter(tapiru::repr_highlighter::instance());
 *   con.print("Connect to https://example.com on 192.168.1.1 port 8080");
 *
 *   // Custom highlighter:
 *   tapiru::regex_highlighter hl;
 *   hl.add_rule(R"(\b[A-Z_]{2,}\b)", tapiru::style{tapiru::colors::yellow});
 *   con.set_highlighter(hl);
 *
 *   // Chain multiple highlighters:
 *   tapiru::highlight_chain chain;
 *   chain.add(tapiru::repr_highlighter::instance());
 *   chain.add(my_custom_highlighter);
 *   con.set_highlighter(chain);
 */

#include "tapiru/core/style.h"
#include "tapiru/exports.h"
#include "tapiru/text/markup.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tapiru {

// ── Highlight span ──────────────────────────────────────────────────────

/**
 * @brief A region of text matched by a highlighter, with its associated style.
 *
 * Spans are non-overlapping and sorted by offset after merging.
 */
struct highlight_span {
    size_t offset = 0;
    size_t length = 0;
    style sty{};
    std::string link; ///< Optional OSC 8 hyperlink URL (empty = no link)

    [[nodiscard]] constexpr size_t end() const noexcept { return offset + length; }
};

/**
 * @brief A text fragment with an optional hyperlink.
 *
 * Extends text_fragment with a link URL for OSC 8 hyperlink emission.
 * When link is non-empty, the terminal will render the text as clickable.
 */
struct linked_fragment {
    std::string text;
    style sty;
    std::string link; ///< OSC 8 hyperlink URL (empty = no link)
};

// ── Highlighter base class ──────────────────────────────────────────────

/**
 * @brief Abstract base for text highlighters.
 *
 * Subclasses implement highlight() to scan plain text and return
 * styled spans for detected patterns.
 */
class TAPIRU_API highlighter {
  public:
    virtual ~highlighter() = default;

    /**
     * @brief Scan text and return highlight spans.
     *
     * Returned spans may overlap — the caller (highlight_chain or console)
     * is responsible for merging. Earlier spans take priority.
     *
     * @param text  plain text to scan (no markup)
     * @return vector of highlight_span, not necessarily sorted
     */
    [[nodiscard]] virtual std::vector<highlight_span> highlight(std::string_view text) const = 0;
};

// ── Regex highlighter ───────────────────────────────────────────────────

/**
 * @brief A highlight rule: regex pattern → style.
 */
struct highlight_rule {
    std::string pattern;
    style sty;
    int group = 0; ///< Capture group to highlight (0 = whole match)
};

/**
 * @brief User-defined regex-based highlighter.
 *
 * Add rules with add_rule(), then pass to console::set_highlighter().
 * Rules are evaluated in order; first match at each position wins.
 */
class TAPIRU_API regex_highlighter final : public highlighter {
  public:
    regex_highlighter() = default;

    /**
     * @brief Add a highlight rule.
     * @param pattern  ECMAScript regex pattern
     * @param sty      style to apply to matches
     * @param group    capture group to highlight (0 = whole match)
     */
    void add_rule(std::string pattern, style sty, int group = 0);

    /**
     * @brief Add a rule matching a literal word (word-boundary wrapped).
     * @param word  exact word to match
     * @param sty   style to apply
     */
    void add_word(std::string_view word, style sty);

    [[nodiscard]] const std::vector<highlight_rule> &rules() const noexcept { return rules_; }

    [[nodiscard]] std::vector<highlight_span> highlight(std::string_view text) const override;

  private:
    std::vector<highlight_rule> rules_;
};

// ── Repr highlighter ────────────────────────────────────────────────────

/**
 * @brief Built-in highlighter with rich pattern recognition.
 *
 * Recognizes: numbers, strings, URLs, paths, UUIDs, IPs, booleans/null,
 * environment variables, email addresses, ISO dates, semantic keywords.
 *
 * Styles are customizable via the nested `theme` struct.
 * Use repr_highlighter::instance() for a shared default instance.
 */
class TAPIRU_API repr_highlighter final : public highlighter {
  public:
    /**
     * @brief Style theme for repr_highlighter.
     *
     * All fields have sensible defaults. Override any field to customize.
     */
    struct theme {
        style number = {colors::cyan, {}, attr::none};
        style number_hex = {colors::bright_cyan, {}, attr::none};
        style string = {colors::green, {}, attr::none};
        style url = {colors::bright_blue, {}, attr::underline};
        style path = {colors::bright_green, {}, attr::italic};
        style uuid = {colors::bright_magenta, {}, attr::none};
        style ipv4 = {colors::bright_yellow, {}, attr::none};
        style ipv6 = {colors::bright_yellow, {}, attr::none};
        style boolean = {colors::bright_cyan, {}, attr::italic};
        style null = {colors::bright_cyan, {}, attr::italic | attr::dim};
        style env_var = {colors::yellow, {}, attr::none};
        style email = {colors::bright_blue, {}, attr::underline};
        style iso_date = {colors::bright_magenta, {}, attr::none};
        style keyword_todo = {colors::black, colors::yellow, attr::bold};
        style keyword_fixme = {colors::black, colors::red, attr::bold};
        style keyword_note = {colors::black, colors::cyan, attr::bold};
        style keyword_hack = {colors::black, colors::magenta, attr::bold};
        style keyword_bug = {colors::white, colors::red, attr::bold};
        style keyword_xxx = {colors::black, colors::yellow, attr::bold};
    };

    /** @brief Construct with default theme. */
    repr_highlighter();

    /** @brief Construct with custom theme. */
    explicit repr_highlighter(theme t);

    /** @brief Access the current theme. */
    [[nodiscard]] const theme &get_theme() const noexcept { return theme_; }

    /** @brief Modify the theme. */
    void set_theme(theme t) noexcept { theme_ = t; }

    /** @brief Get a shared default instance (thread-safe after init). */
    [[nodiscard]] static const repr_highlighter &instance();

    [[nodiscard]] std::vector<highlight_span> highlight(std::string_view text) const override;

  private:
    theme theme_;
};

// ── Highlight chain ─────────────────────────────────────────────────────

/**
 * @brief Composes multiple highlighters into a pipeline.
 *
 * Each highlighter's spans are merged in order. Earlier highlighters
 * have priority: if two spans overlap, the first one wins.
 */
class TAPIRU_API highlight_chain final : public highlighter {
  public:
    highlight_chain() = default;

    /**
     * @brief Add a highlighter to the chain (by reference — caller owns lifetime).
     */
    void add(const highlighter &hl);

    /**
     * @brief Add a highlighter to the chain (by shared_ptr — chain co-owns).
     */
    void add(std::shared_ptr<highlighter> hl);

    [[nodiscard]] std::vector<highlight_span> highlight(std::string_view text) const override;

  private:
    std::vector<const highlighter *> refs_;
    std::vector<std::shared_ptr<highlighter>> owned_;
};

// ── Utility functions ───────────────────────────────────────────────────

/**
 * @brief Merge overlapping spans, keeping earlier spans on conflict.
 *
 * Input spans need not be sorted. Output is sorted by offset,
 * non-overlapping, and gaps are preserved (unstyled).
 */
[[nodiscard]] TAPIRU_API std::vector<highlight_span> merge_spans(std::vector<highlight_span> spans, size_t text_length);

/**
 * @brief Apply highlight spans to produce styled text fragments.
 *
 * Regions not covered by any span get the default style.
 * This is the bridge between the highlighter output and the
 * existing text_fragment / console rendering pipeline.
 *
 * @param text   the plain text
 * @param spans  merged, non-overlapping spans (from merge_spans)
 * @return vector of text_fragment ready for console emission
 */
[[nodiscard]] TAPIRU_API std::vector<text_fragment> apply_highlights(std::string_view text,
                                                                     const std::vector<highlight_span> &spans);

/**
 * @brief Apply highlight spans to produce linked text fragments.
 *
 * Like apply_highlights(), but preserves link URLs from spans
 * for OSC 8 hyperlink emission.
 */
[[nodiscard]] TAPIRU_API std::vector<linked_fragment> apply_highlights_linked(std::string_view text,
                                                                              const std::vector<highlight_span> &spans);

/**
 * @brief Convenience: highlight text and produce fragments in one call.
 *
 * @param text  plain text
 * @param hl    highlighter to use
 * @return styled text fragments
 */
[[nodiscard]] TAPIRU_API std::vector<text_fragment> highlight_text(std::string_view text, const highlighter &hl);

/**
 * @brief Convenience: highlight text and produce linked fragments in one call.
 *
 * Like highlight_text(), but preserves link URLs for OSC 8 hyperlinks.
 */
[[nodiscard]] TAPIRU_API std::vector<linked_fragment> highlight_text_linked(std::string_view text,
                                                                            const highlighter &hl);

} // namespace tapiru
