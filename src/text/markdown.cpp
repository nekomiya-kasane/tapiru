/**
 * @file markdown.cpp
 * @brief Basic Markdown parser and widget builder.
 */

#include "tapiru/text/markdown.h"

#include "detail/scene.h"
#include "tapiru/widgets/builders.h"

#include <algorithm>
#include <string>
#include <unordered_set>

namespace tapiru {

    markdown_builder &markdown_builder::key(std::string_view k) {
        key_ = detail::fnv1a_hash(k);
        return *this;
    }

    // ── Block-level parser ──────────────────────────────────────────────────

    std::vector<md_block> parse_markdown(std::string_view md) {
        std::vector<md_block> blocks;

        size_t i = 0;
        while (i < md.size()) {
            // Find end of line
            size_t eol = md.find('\n', i);
            if (eol == std::string_view::npos) {
                eol = md.size();
            }
            auto line = md.substr(i, eol - i);
            i = eol + 1;

            // Skip empty lines
            if (line.empty() || (line.size() == 1 && line[0] == '\r')) {
                continue;
            }

            // Strip trailing \r
            if (!line.empty() && line.back() == '\r') {
                line = line.substr(0, line.size() - 1);
            }
            if (line.empty()) {
                continue;
            }

            // Horizontal rule: --- or *** or ___  (3+ chars)
            if (line.size() >= 3) {
                bool is_rule = true;
                char rc = line[0];
                if (rc == '-' || rc == '*' || rc == '_') {
                    for (size_t j = 0; j < line.size(); ++j) {
                        if (line[j] != rc && line[j] != ' ') {
                            is_rule = false;
                            break;
                        }
                    }
                    if (is_rule) {
                        blocks.push_back({md_block_type::rule, 0, ""});
                        continue;
                    }
                }
            }

            // Heading: # H1, ## H2, ### H3
            if (line[0] == '#') {
                uint8_t level = 0;
                size_t pos = 0;
                while (pos < line.size() && line[pos] == '#' && level < 6) {
                    ++level;
                    ++pos;
                }
                if (pos < line.size() && line[pos] == ' ') {
                    ++pos;
                }
                blocks.push_back({md_block_type::heading, level, std::string(line.substr(pos))});
                continue;
            }

            // Blockquote: > text
            if (line.size() >= 2 && line[0] == '>' && line[1] == ' ') {
                blocks.push_back({md_block_type::blockquote, 0, std::string(line.substr(2))});
                continue;
            }
            if (line.size() >= 1 && line[0] == '>') {
                blocks.push_back({md_block_type::blockquote, 0, std::string(line.substr(1))});
                continue;
            }

            // Fenced code block: ```lang
            if (line.size() >= 3 && line[0] == '`' && line[1] == '`' && line[2] == '`') {
                std::string lang;
                if (line.size() > 3) {
                    lang = std::string(line.substr(3));
                    // Trim whitespace
                    while (!lang.empty() && (lang.back() == ' ' || lang.back() == '\r')) {
                        lang.pop_back();
                    }
                }
                // Collect lines until closing ```
                std::string code_content;
                while (i < md.size()) {
                    size_t ceol = md.find('\n', i);
                    if (ceol == std::string_view::npos) {
                        ceol = md.size();
                    }
                    auto cline = md.substr(i, ceol - i);
                    i = ceol + 1;
                    if (!cline.empty() && cline.back() == '\r') {
                        cline = cline.substr(0, cline.size() - 1);
                    }
                    if (cline.size() >= 3 && cline[0] == '`' && cline[1] == '`' && cline[2] == '`') {
                        break;
                    }
                    if (!code_content.empty()) {
                        code_content += '\n';
                    }
                    code_content += cline;
                }
                md_block blk;
                blk.type = md_block_type::code_block;
                blk.content = std::move(code_content);
                blk.language = std::move(lang);
                blocks.push_back(std::move(blk));
                continue;
            }

            // GFM table: | col1 | col2 |
            if (line.size() >= 3 && line[0] == '|') {
                // Parse this line and subsequent table lines
                md_block tblk;
                tblk.type = md_block_type::table;
                auto parse_row = [](std::string_view row) -> std::vector<std::string> {
                    std::vector<std::string> cells;
                    size_t p = 0;
                    if (p < row.size() && row[p] == '|') {
                        ++p;
                    }
                    while (p < row.size()) {
                        size_t pipe = row.find('|', p);
                        if (pipe == std::string_view::npos) {
                            break;
                        }
                        auto cell = row.substr(p, pipe - p);
                        // Trim
                        while (!cell.empty() && cell.front() == ' ') {
                            cell = cell.substr(1);
                        }
                        while (!cell.empty() && cell.back() == ' ') {
                            cell = cell.substr(0, cell.size() - 1);
                        }
                        cells.push_back(std::string(cell));
                        p = pipe + 1;
                    }
                    return cells;
                };
                tblk.table_rows.push_back(parse_row(line));
                // Continue reading table lines
                while (i < md.size()) {
                    size_t teol = md.find('\n', i);
                    if (teol == std::string_view::npos) {
                        teol = md.size();
                    }
                    auto tline = md.substr(i, teol - i);
                    if (!tline.empty() && tline.back() == '\r') {
                        tline = tline.substr(0, tline.size() - 1);
                    }
                    if (tline.empty() || tline[0] != '|') {
                        break;
                    }
                    i = teol + 1;
                    // Skip separator rows (|---|---| etc)
                    bool is_sep = true;
                    for (size_t j = 0; j < tline.size(); ++j) {
                        char c = tline[j];
                        if (c != '|' && c != '-' && c != ':' && c != ' ') {
                            is_sep = false;
                            break;
                        }
                    }
                    if (is_sep) {
                        continue;
                    }
                    tblk.table_rows.push_back(parse_row(tline));
                }
                blocks.push_back(std::move(tblk));
                continue;
            }

            // Task list: - [x] or - [ ]
            if (line.size() >= 5 && line[0] == '-' && line[1] == ' ' && line[2] == '[' &&
                (line[3] == 'x' || line[3] == 'X' || line[3] == ' ') && line[4] == ']') {
                md_block blk;
                blk.type = md_block_type::task_list;
                blk.checked = (line[3] == 'x' || line[3] == 'X');
                blk.content = std::string(line.substr(5));
                if (!blk.content.empty() && blk.content[0] == ' ') {
                    blk.content = blk.content.substr(1);
                }
                blocks.push_back(std::move(blk));
                continue;
            }

            // Unordered list: - item or * item
            if (line.size() >= 2 && (line[0] == '-' || line[0] == '*') && line[1] == ' ') {
                blocks.push_back({md_block_type::list_item, 0, std::string(line.substr(2))});
                continue;
            }

            // Default: paragraph
            blocks.push_back({md_block_type::paragraph, 0, std::string(line)});
        }

        return blocks;
    }

    // ── Inline markdown → markup tags ───────────────────────────────────────

    std::string md_inline_to_markup(std::string_view text) {
        std::string result;
        result.reserve(text.size() + 32);

        size_t i = 0;
        while (i < text.size()) {
            // Bold: **text**
            if (i + 1 < text.size() && text[i] == '*' && text[i + 1] == '*') {
                size_t close = text.find("**", i + 2);
                if (close != std::string_view::npos) {
                    result += "[bold]";
                    result += text.substr(i + 2, close - i - 2);
                    result += "[/bold]";
                    i = close + 2;
                    continue;
                }
            }

            // Italic: *text*
            if (text[i] == '*' && (i + 1 < text.size()) && text[i + 1] != '*') {
                size_t close = text.find('*', i + 1);
                if (close != std::string_view::npos) {
                    result += "[italic]";
                    result += text.substr(i + 1, close - i - 1);
                    result += "[/italic]";
                    i = close + 1;
                    continue;
                }
            }

            // Inline code: `code`
            if (text[i] == '`') {
                size_t close = text.find('`', i + 1);
                if (close != std::string_view::npos) {
                    result += "[bold cyan]";
                    // Escape brackets so content isn't parsed as markup
                    auto code = text.substr(i + 1, close - i - 1);
                    for (char ch : code) {
                        if (ch == '[') {
                            result += "[[";
                        } else {
                            result += ch;
                        }
                    }
                    result += "[/bold cyan]";
                    i = close + 1;
                    continue;
                }
            }

            result += text[i];
            ++i;
        }

        return result;
    }

    // ── markdown_builder ────────────────────────────────────────────────────

    markdown_builder::markdown_builder(std::string_view md, uint32_t max_depth)
        : blocks_(parse_markdown(md)), max_depth_(max_depth) {}

    node_id markdown_builder::flatten_into(detail::scene &s) const {
        // Build a single text node with all blocks concatenated via newlines,
        // each block styled via markup conversion.
        detail::text_data td;

        for (size_t bi = 0; bi < blocks_.size(); ++bi) {
            if (bi > 0) {
                td.fragments.push_back({"\n", style{}});
            }

            const auto &blk = blocks_[bi];

            switch (blk.type) {
            case md_block_type::heading: {
                // Headings: bold, with level-based color
                std::string markup = "[bold";
                if (blk.level == 1) {
                    markup += " bright_white on_blue";
                } else if (blk.level == 2) {
                    markup += " bright_cyan";
                } else {
                    markup += " bright_yellow";
                }
                markup += "]";
                markup += md_inline_to_markup(blk.content);
                markup += "[/]";
                auto frags = parse_markup(markup);
                for (auto &f : frags) {
                    td.fragments.push_back(std::move(f));
                }
                break;
            }
            case md_block_type::rule: {
                // Use em-dash (U+2014) to avoid border-join merging with adjacent │
                td.fragments.push_back({"\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94"
                                        "\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94"
                                        "\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94"
                                        "\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94"
                                        "\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94\xe2\x80\x94"
                                        "\xe2\x80\x94\xe2\x80\x94",
                                        style{colors::bright_black, {}, attr::dim}});
                break;
            }
            case md_block_type::blockquote: {
                std::string markup = "[dim]\xe2\x94\x82 [/dim]"; // │
                markup += "[italic]";
                markup += md_inline_to_markup(blk.content);
                markup += "[/italic]";
                auto frags = parse_markup(markup);
                for (auto &f : frags) {
                    td.fragments.push_back(std::move(f));
                }
                break;
            }
            case md_block_type::list_item: {
                std::string markup = " [bold]\xe2\x80\xa2[/bold] "; // •
                markup += md_inline_to_markup(blk.content);
                auto frags = parse_markup(markup);
                for (auto &f : frags) {
                    td.fragments.push_back(std::move(f));
                }
                break;
            }
            case md_block_type::code_block: {
                // Render code block with syntax highlighting
                std::string highlighted;
                if (!blk.language.empty()) {
                    highlighted = syntax_highlight(blk.content, blk.language);
                } else {
                    // Escape brackets in raw code
                    for (char ch : blk.content) {
                        if (ch == '[') {
                            highlighted += "[[";
                        } else {
                            highlighted += ch;
                        }
                    }
                }
                std::string markup = "[on_bright_black] ";
                markup += highlighted;
                markup += " [/]";
                auto frags = parse_markup(markup);
                for (auto &f : frags) {
                    td.fragments.push_back(std::move(f));
                }
                break;
            }
            case md_block_type::task_list: {
                std::string markup;
                if (blk.checked) {
                    markup = " [bold green]\xe2\x9c\x93[/bold green] "; // ✓
                } else {
                    markup = " [dim]\xe2\x98\x90[/dim] "; // ☐
                }
                markup += md_inline_to_markup(blk.content);
                auto frags = parse_markup(markup);
                for (auto &f : frags) {
                    td.fragments.push_back(std::move(f));
                }
                break;
            }
            case md_block_type::table: {
                // Render table as pipe-separated text with header styling
                bool first_row = true;
                for (const auto &row : blk.table_rows) {
                    if (!first_row) {
                        td.fragments.push_back({"\n", style{}});
                    }
                    std::string markup;
                    for (size_t ci = 0; ci < row.size(); ++ci) {
                        if (ci > 0) {
                            markup += " | ";
                        }
                        if (first_row) {
                            markup += "[bold]" + row[ci] + "[/bold]";
                        } else {
                            markup += row[ci];
                        }
                    }
                    auto frags = parse_markup(markup);
                    for (auto &f : frags) {
                        td.fragments.push_back(std::move(f));
                    }
                    first_row = false;
                }
                break;
            }
            case md_block_type::paragraph:
            default: {
                std::string markup = md_inline_to_markup(blk.content);
                auto frags = parse_markup(markup);
                for (auto &f : frags) {
                    td.fragments.push_back(std::move(f));
                }
                break;
            }
            }
        }

        td.overflow = overflow_mode::truncate;
        auto pi = s.add_text(std::move(td));
        return s.add_node(detail::widget_type::text, pi);
    }

    // ── Syntax highlighting ────────────────────────────────────────────────

    std::string syntax_highlight(std::string_view code, std::string_view language) {
        // Keyword sets for supported languages
        static const std::unordered_set<std::string> cpp_keywords = {
            "auto",     "break",    "case",      "catch",     "class",    "const",     "constexpr", "continue",
            "default",  "delete",   "do",        "else",      "enum",     "explicit",  "extern",    "false",
            "for",      "friend",   "goto",      "if",        "inline",   "namespace", "new",       "noexcept",
            "nullptr",  "operator", "private",   "protected", "public",   "return",    "sizeof",    "static",
            "struct",   "switch",   "template",  "this",      "throw",    "true",      "try",       "typedef",
            "typename", "using",    "virtual",   "void",      "volatile", "while",     "int",       "float",
            "double",   "char",     "bool",      "long",      "short",    "unsigned",  "signed",    "include",
            "define",   "ifdef",    "ifndef",    "endif",     "pragma",   "override",  "final",     "concept",
            "requires", "co_await", "co_return", "co_yield",
        };
        static const std::unordered_set<std::string> py_keywords = {
            "and",  "as",     "assert", "async",  "await",  "break",   "class",    "continue", "def",
            "del",  "elif",   "else",   "except", "False",  "finally", "for",      "from",     "global",
            "if",   "import", "in",     "is",     "lambda", "None",    "nonlocal", "not",      "or",
            "pass", "raise",  "return", "True",   "try",    "while",   "with",     "yield",
        };
        static const std::unordered_set<std::string> js_keywords = {
            "async",    "await",   "break",    "case",      "catch",  "class",  "const",      "continue",
            "debugger", "default", "delete",   "do",        "else",   "export", "extends",    "false",
            "finally",  "for",     "function", "if",        "import", "in",     "instanceof", "let",
            "new",      "null",    "of",       "return",    "super",  "switch", "this",       "throw",
            "true",     "try",     "typeof",   "undefined", "var",    "void",   "while",      "yield",
        };

        const std::unordered_set<std::string> *kw_set = nullptr;
        if (language == "cpp" || language == "c++" || language == "c" || language == "h") {
            kw_set = &cpp_keywords;
        } else if (language == "python" || language == "py") {
            kw_set = &py_keywords;
        } else if (language == "javascript" || language == "js" || language == "typescript" || language == "ts") {
            kw_set = &js_keywords;
        }

        if (!kw_set) {
            // No highlighting — just escape brackets
            std::string result;
            for (char ch : code) {
                if (ch == '[') {
                    result += "[[";
                } else {
                    result += ch;
                }
            }
            return result;
        }

        std::string result;
        size_t i = 0;
        while (i < code.size()) {
            // String literals
            if (code[i] == '"' || code[i] == '\'') {
                char quote = code[i];
                result += "[green]";
                result += code[i++];
                while (i < code.size() && code[i] != quote) {
                    if (code[i] == '\\' && i + 1 < code.size()) {
                        result += code[i++];
                    }
                    if (code[i] == '[') {
                        result += "[[";
                    } else {
                        result += code[i];
                    }
                    ++i;
                }
                if (i < code.size()) {
                    result += code[i++];
                }
                result += "[/green]";
                continue;
            }

            // Comments: // or #
            if ((code[i] == '/' && i + 1 < code.size() && code[i + 1] == '/') ||
                (code[i] == '#' && (language == "python" || language == "py"))) {
                result += "[dim]";
                while (i < code.size() && code[i] != '\n') {
                    if (code[i] == '[') {
                        result += "[[";
                    } else {
                        result += code[i];
                    }
                    ++i;
                }
                result += "[/dim]";
                continue;
            }

            // Numbers
            if (std::isdigit(static_cast<unsigned char>(code[i]))) {
                result += "[bright_magenta]";
                while (i < code.size() && (std::isdigit(static_cast<unsigned char>(code[i])) || code[i] == '.' ||
                                           code[i] == 'x' || code[i] == 'f')) {
                    result += code[i++];
                }
                result += "[/bright_magenta]";
                continue;
            }

            // Identifiers / keywords
            if (std::isalpha(static_cast<unsigned char>(code[i])) || code[i] == '_') {
                std::string word;
                while (i < code.size() && (std::isalnum(static_cast<unsigned char>(code[i])) || code[i] == '_')) {
                    word += code[i++];
                }
                if (kw_set->count(word)) {
                    result += "[bold bright_blue]" + word + "[/bold bright_blue]";
                } else {
                    result += word;
                }
                continue;
            }

            // Escape brackets
            if (code[i] == '[') {
                result += "[[";
            } else {
                result += code[i];
            }
            ++i;
        }

        return result;
    }

} // namespace tapiru
