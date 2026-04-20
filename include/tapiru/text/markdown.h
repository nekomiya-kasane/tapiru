#pragma once

/**
 * @file markdown.h
 * @brief Basic Markdown → widget builder conversion.
 *
 * Supports a subset of Markdown:
 *   - Headings: # H1, ## H2, ### H3
 *   - Bold: **text**
 *   - Italic: *text*
 *   - Code: `code`
 *   - Horizontal rule: --- or ***
 *   - Blockquote: > text
 *   - Unordered list: - item or * item
 *
 * Usage:
 *   auto builder = tapiru::markdown_to_widget("# Hello\nSome **bold** text");
 *   con.print_widget(builder, 80);
 */

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "tapiru/exports.h"
#include "tapiru/text/markup.h"

namespace tapiru {

namespace detail { class scene; }
using node_id = uint32_t;

// ── Markdown AST ────────────────────────────────────────────────────────

enum class md_block_type : uint8_t {
    paragraph,
    heading,
    rule,
    blockquote,
    list_item,
    code_block,
    task_list,
    table,
};

struct md_block {
    md_block_type type = md_block_type::paragraph;
    uint8_t       level = 0;  // heading level (1-3), or 0
    std::string   content;    // inline markdown content
    std::string   language;   // code_block language tag
    bool          checked = false;  // task_list checked state
    std::vector<std::vector<std::string>> table_rows;  // table rows×cols
};

/**
 * @brief Parse markdown text into a list of blocks.
 */
[[nodiscard]] TAPIRU_API std::vector<md_block> parse_markdown(std::string_view md);

/**
 * @brief Convert inline markdown (bold, italic, code) to markup tags.
 *
 * Transforms **bold** → [bold]bold[/bold], *italic* → [italic]italic[/italic],
 * `code` → [bold cyan]code[/bold cyan].
 */
[[nodiscard]] TAPIRU_API std::string md_inline_to_markup(std::string_view text);

// ── Markdown builder ────────────────────────────────────────────────────

/**
 * @brief A builder that renders parsed markdown blocks into the scene.
 *
 * Flattens markdown into text nodes with appropriate styling.
 */
class TAPIRU_API markdown_builder {
public:
    explicit markdown_builder(std::string_view md, uint32_t max_depth = 32);

    markdown_builder& key(std::string_view k);
    markdown_builder& z_order(int16_t z) { z_order_ = z; return *this; }

    node_id flatten_into(detail::scene& s) const;

private:
    std::vector<md_block> blocks_;
    uint32_t max_depth_ = 32;
    uint64_t key_       = 0;
    int16_t  z_order_   = 0;
};

// ── Syntax highlighting ────────────────────────────────────────────────

/**
 * @brief Apply keyword-level syntax highlighting to source code.
 *
 * Returns markup-tagged text with language keywords colored.
 * Supported languages: cpp, python, javascript.
 */
[[nodiscard]] TAPIRU_API std::string syntax_highlight(std::string_view code, std::string_view language);

}  // namespace tapiru
