#include <gtest/gtest.h>

#include <mutex>
#include <string>

#include "tapiru/core/console.h"
#include "tapiru/text/markdown.h"

using namespace tapiru;

// ── parse_markdown ──────────────────────────────────────────────────────

TEST(MarkdownParserTest, Heading) {
    auto blocks = parse_markdown("# Title");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::heading);
    EXPECT_EQ(blocks[0].level, 1);
    EXPECT_EQ(blocks[0].content, "Title");
}

TEST(MarkdownParserTest, HeadingLevels) {
    auto blocks = parse_markdown("# H1\n## H2\n### H3");
    ASSERT_EQ(blocks.size(), 3u);
    EXPECT_EQ(blocks[0].level, 1);
    EXPECT_EQ(blocks[1].level, 2);
    EXPECT_EQ(blocks[2].level, 3);
}

TEST(MarkdownParserTest, HorizontalRule) {
    auto blocks = parse_markdown("---");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::rule);
}

TEST(MarkdownParserTest, HorizontalRuleStars) {
    auto blocks = parse_markdown("***");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::rule);
}

TEST(MarkdownParserTest, Blockquote) {
    auto blocks = parse_markdown("> Quote text");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::blockquote);
    EXPECT_EQ(blocks[0].content, "Quote text");
}

TEST(MarkdownParserTest, ListItem) {
    auto blocks = parse_markdown("- Item one\n- Item two");
    ASSERT_EQ(blocks.size(), 2u);
    EXPECT_EQ(blocks[0].type, md_block_type::list_item);
    EXPECT_EQ(blocks[0].content, "Item one");
    EXPECT_EQ(blocks[1].type, md_block_type::list_item);
    EXPECT_EQ(blocks[1].content, "Item two");
}

TEST(MarkdownParserTest, Paragraph) {
    auto blocks = parse_markdown("Just some text");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::paragraph);
    EXPECT_EQ(blocks[0].content, "Just some text");
}

TEST(MarkdownParserTest, EmptyInput) {
    auto blocks = parse_markdown("");
    EXPECT_TRUE(blocks.empty());
}

TEST(MarkdownParserTest, MixedBlocks) {
    auto blocks = parse_markdown("# Title\n\nSome text\n\n---\n\n- Item");
    ASSERT_GE(blocks.size(), 4u);
    EXPECT_EQ(blocks[0].type, md_block_type::heading);
    EXPECT_EQ(blocks[1].type, md_block_type::paragraph);
    EXPECT_EQ(blocks[2].type, md_block_type::rule);
    EXPECT_EQ(blocks[3].type, md_block_type::list_item);
}

// ── md_inline_to_markup ─────────────────────────────────────────────────

TEST(MarkdownInlineTest, Bold) {
    auto result = md_inline_to_markup("Hello **world**");
    EXPECT_TRUE(result.find("[bold]") != std::string::npos);
    EXPECT_TRUE(result.find("world") != std::string::npos);
    EXPECT_TRUE(result.find("[/bold]") != std::string::npos);
}

TEST(MarkdownInlineTest, Italic) {
    auto result = md_inline_to_markup("Hello *world*");
    EXPECT_TRUE(result.find("[italic]") != std::string::npos);
    EXPECT_TRUE(result.find("world") != std::string::npos);
}

TEST(MarkdownInlineTest, InlineCode) {
    auto result = md_inline_to_markup("Use `printf`");
    EXPECT_TRUE(result.find("[bold cyan]") != std::string::npos);
    EXPECT_TRUE(result.find("printf") != std::string::npos);
}

TEST(MarkdownInlineTest, PlainText) {
    auto result = md_inline_to_markup("No formatting here");
    EXPECT_EQ(result, "No formatting here");
}

TEST(MarkdownInlineTest, Mixed) {
    auto result = md_inline_to_markup("**bold** and *italic* and `code`");
    EXPECT_TRUE(result.find("[bold]") != std::string::npos);
    EXPECT_TRUE(result.find("[italic]") != std::string::npos);
    EXPECT_TRUE(result.find("[bold cyan]") != std::string::npos);
}

// ── markdown_builder rendering ──────────────────────────────────────────

class capture_sink {
public:
    void operator()(std::string_view data) {
        std::lock_guard lk(mu_);
        buffer_ += data;
    }
    [[nodiscard]] std::string snapshot() const {
        std::lock_guard lk(mu_);
        return buffer_;
    }
private:
    mutable std::mutex mu_;
    std::string buffer_;
};

TEST(MarkdownBuilderTest, RenderHeading) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    con.print_widget(markdown_builder("# Hello World"), 80);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Hello World") != std::string::npos);
}

TEST(MarkdownBuilderTest, RenderMixed) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    con.print_widget(markdown_builder("# Title\n\nSome **bold** text\n\n- Item one\n- Item two"), 80);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Title") != std::string::npos);
    EXPECT_TRUE(out.find("bold") != std::string::npos);
    EXPECT_TRUE(out.find("Item one") != std::string::npos);
    EXPECT_TRUE(out.find("Item two") != std::string::npos);
}

TEST(MarkdownBuilderTest, RenderRule) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    con.print_widget(markdown_builder("Above\n---\nBelow"), 80);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Above") != std::string::npos);
    EXPECT_TRUE(out.find("Below") != std::string::npos);
}

// ── GFM: Task lists ────────────────────────────────────────────────────

TEST(MarkdownParserTest, TaskListChecked) {
    auto blocks = parse_markdown("- [x] Done task");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::task_list);
    EXPECT_TRUE(blocks[0].checked);
    EXPECT_EQ(blocks[0].content, "Done task");
}

TEST(MarkdownParserTest, TaskListUnchecked) {
    auto blocks = parse_markdown("- [ ] Pending task");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::task_list);
    EXPECT_FALSE(blocks[0].checked);
    EXPECT_EQ(blocks[0].content, "Pending task");
}

TEST(MarkdownParserTest, TaskListMultiple) {
    auto blocks = parse_markdown("- [x] A\n- [ ] B\n- [X] C");
    ASSERT_EQ(blocks.size(), 3u);
    EXPECT_TRUE(blocks[0].checked);
    EXPECT_FALSE(blocks[1].checked);
    EXPECT_TRUE(blocks[2].checked);
}

TEST(MarkdownBuilderTest, RenderTaskList) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    con.print_widget(markdown_builder("- [x] Done\n- [ ] Todo"), 80);
    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Done") != std::string::npos);
    EXPECT_TRUE(out.find("Todo") != std::string::npos);
}

// ── GFM: Fenced code blocks ───────────────────────────────────────────

TEST(MarkdownParserTest, FencedCodeBlock) {
    auto blocks = parse_markdown("```cpp\nint x = 42;\n```");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::code_block);
    EXPECT_EQ(blocks[0].language, "cpp");
    EXPECT_EQ(blocks[0].content, "int x = 42;");
}

TEST(MarkdownParserTest, FencedCodeBlockNoLang) {
    auto blocks = parse_markdown("```\nhello\nworld\n```");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::code_block);
    EXPECT_TRUE(blocks[0].language.empty());
    EXPECT_EQ(blocks[0].content, "hello\nworld");
}

TEST(MarkdownBuilderTest, RenderCodeBlock) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    con.print_widget(markdown_builder("```\nsome code\n```"), 80);
    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("some code") != std::string::npos);
}

// ── GFM: Tables ───────────────────────────────────────────────────────

TEST(MarkdownParserTest, GfmTable) {
    auto blocks = parse_markdown("| A | B |\n|---|---|\n| 1 | 2 |\n| 3 | 4 |");
    ASSERT_EQ(blocks.size(), 1u);
    EXPECT_EQ(blocks[0].type, md_block_type::table);
    ASSERT_EQ(blocks[0].table_rows.size(), 3u);  // header + 2 data rows
    EXPECT_EQ(blocks[0].table_rows[0][0], "A");
    EXPECT_EQ(blocks[0].table_rows[0][1], "B");
    EXPECT_EQ(blocks[0].table_rows[1][0], "1");
    EXPECT_EQ(blocks[0].table_rows[2][1], "4");
}

TEST(MarkdownBuilderTest, RenderTable) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    con.print_widget(markdown_builder("| Name | Age |\n|---|---|\n| Alice | 30 |"), 80);
    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Name") != std::string::npos);
    EXPECT_TRUE(out.find("Alice") != std::string::npos);
}

// ── Syntax highlighting ───────────────────────────────────────────────

TEST(SyntaxHighlightTest, CppKeywords) {
    auto result = syntax_highlight("int x = 42;", "cpp");
    EXPECT_TRUE(result.find("int") != std::string::npos);
    EXPECT_TRUE(result.find("bright_blue") != std::string::npos);
    EXPECT_TRUE(result.find("bright_magenta") != std::string::npos);
}

TEST(SyntaxHighlightTest, PythonKeywords) {
    auto result = syntax_highlight("def foo():\n    return True", "python");
    EXPECT_TRUE(result.find("def") != std::string::npos);
    EXPECT_TRUE(result.find("return") != std::string::npos);
    EXPECT_TRUE(result.find("bright_blue") != std::string::npos);
}

TEST(SyntaxHighlightTest, StringLiterals) {
    auto result = syntax_highlight("x = \"hello\"", "cpp");
    EXPECT_TRUE(result.find("green") != std::string::npos);
}

TEST(SyntaxHighlightTest, UnknownLanguagePassthrough) {
    auto result = syntax_highlight("foo bar", "unknown");
    EXPECT_EQ(result, "foo bar");
}

TEST(SyntaxHighlightTest, BracketEscaping) {
    auto result = syntax_highlight("a[0]", "unknown");
    EXPECT_TRUE(result.find("[[") != std::string::npos);
}
