/**
 * @file highlighter_test.cpp
 * @brief Tests for the auto-highlight system.
 */

#include "tapiru/core/console.h"
#include "tapiru/text/highlighter.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
// highlight_span basics
// ═══════════════════════════════════════════════════════════════════════

TEST(HighlightSpan, EndCalculation) {
    highlight_span s{10, 5, {}};
    EXPECT_EQ(s.end(), 15u);
}

// ═══════════════════════════════════════════════════════════════════════
// merge_spans
// ═══════════════════════════════════════════════════════════════════════

TEST(MergeSpans, EmptyInput) {
    auto result = merge_spans({}, 100);
    EXPECT_TRUE(result.empty());
}

TEST(MergeSpans, NonOverlapping) {
    std::vector<highlight_span> spans = {
        {0, 3, {colors::red}},
        {5, 2, {colors::blue}},
    };
    auto merged = merge_spans(spans, 10);
    ASSERT_EQ(merged.size(), 2u);
    EXPECT_EQ(merged[0].offset, 0u);
    EXPECT_EQ(merged[0].length, 3u);
    EXPECT_EQ(merged[1].offset, 5u);
    EXPECT_EQ(merged[1].length, 2u);
}

TEST(MergeSpans, OverlappingEarlierWins) {
    std::vector<highlight_span> spans = {
        {0, 5, {colors::red}},
        {3, 5, {colors::blue}},
    };
    auto merged = merge_spans(spans, 10);
    ASSERT_EQ(merged.size(), 2u);
    // First span: 0..5 red
    EXPECT_EQ(merged[0].offset, 0u);
    EXPECT_EQ(merged[0].length, 5u);
    EXPECT_EQ(merged[0].sty.fg, colors::red);
    // Second span: 5..8 blue (trimmed)
    EXPECT_EQ(merged[1].offset, 5u);
    EXPECT_EQ(merged[1].length, 3u);
    EXPECT_EQ(merged[1].sty.fg, colors::blue);
}

TEST(MergeSpans, FullyOverlappedSkipped) {
    std::vector<highlight_span> spans = {
        {0, 10, {colors::red}},
        {2, 3, {colors::blue}}, // fully inside first
    };
    auto merged = merge_spans(spans, 10);
    ASSERT_EQ(merged.size(), 1u);
    EXPECT_EQ(merged[0].offset, 0u);
    EXPECT_EQ(merged[0].length, 10u);
}

TEST(MergeSpans, ClampToTextLength) {
    std::vector<highlight_span> spans = {
        {5, 100, {colors::red}},
    };
    auto merged = merge_spans(spans, 10);
    ASSERT_EQ(merged.size(), 1u);
    EXPECT_EQ(merged[0].length, 5u);
}

// ═══════════════════════════════════════════════════════════════════════
// apply_highlights
// ═══════════════════════════════════════════════════════════════════════

TEST(ApplyHighlights, NoSpans) {
    auto frags = apply_highlights("hello world", {});
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "hello world");
    EXPECT_TRUE(frags[0].sty.is_default());
}

TEST(ApplyHighlights, SingleSpan) {
    std::vector<highlight_span> spans = {{6, 5, {colors::red}}};
    auto frags = apply_highlights("hello world", spans);
    ASSERT_EQ(frags.size(), 2u);
    EXPECT_EQ(frags[0].text, "hello ");
    EXPECT_TRUE(frags[0].sty.is_default());
    EXPECT_EQ(frags[1].text, "world");
    EXPECT_EQ(frags[1].sty.fg, colors::red);
}

TEST(ApplyHighlights, MultipleSpansWithGaps) {
    std::vector<highlight_span> spans = {
        {0, 5, {colors::red}},
        {6, 5, {colors::blue}},
    };
    auto frags = apply_highlights("hello world", spans);
    ASSERT_EQ(frags.size(), 3u);
    EXPECT_EQ(frags[0].text, "hello");
    EXPECT_EQ(frags[0].sty.fg, colors::red);
    EXPECT_EQ(frags[1].text, " ");
    EXPECT_TRUE(frags[1].sty.is_default());
    EXPECT_EQ(frags[2].text, "world");
    EXPECT_EQ(frags[2].sty.fg, colors::blue);
}

// ═══════════════════════════════════════════════════════════════════════
// regex_highlighter
// ═══════════════════════════════════════════════════════════════════════

TEST(RegexHighlighter, BasicPattern) {
    regex_highlighter hl;
    hl.add_rule(R"(\d+)", style{colors::cyan});

    auto spans = hl.highlight("abc 42 def 100");
    ASSERT_GE(spans.size(), 2u);

    // Find the span for "42"
    bool found_42 = false;
    bool found_100 = false;
    for (const auto &s : spans) {
        if (s.offset == 4 && s.length == 2) found_42 = true;
        if (s.offset == 11 && s.length == 3) found_100 = true;
    }
    EXPECT_TRUE(found_42);
    EXPECT_TRUE(found_100);
}

TEST(RegexHighlighter, AddWord) {
    regex_highlighter hl;
    hl.add_word("TODO", style{colors::yellow, {}, attr::bold});

    auto spans = hl.highlight("fix this TODO item");
    ASSERT_GE(spans.size(), 1u);
    EXPECT_EQ(spans[0].offset, 9u);
    EXPECT_EQ(spans[0].length, 4u);
}

TEST(RegexHighlighter, InvalidPatternSkipped) {
    regex_highlighter hl;
    hl.add_rule("[invalid(", style{colors::red}); // bad regex
    hl.add_rule(R"(\d+)", style{colors::cyan});

    auto spans = hl.highlight("test 42");
    // Should still find the number despite the bad pattern
    ASSERT_GE(spans.size(), 1u);
}

TEST(RegexHighlighter, CaptureGroup) {
    regex_highlighter hl;
    hl.add_rule(R"((\w+)=(\w+))", style{colors::green}, 2); // highlight value only

    auto spans = hl.highlight("key=value");
    ASSERT_GE(spans.size(), 1u);
    EXPECT_EQ(spans[0].offset, 4u);
    EXPECT_EQ(spans[0].length, 5u);
}

// ═══════════════════════════════════════════════════════════════════════
// repr_highlighter
// ═══════════════════════════════════════════════════════════════════════

TEST(ReprHighlighter, DetectsIntegers) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("count is 42 items");
    bool found = false;
    for (const auto &s : spans) {
        if (s.offset == 9 && s.length == 2) {
            found = true;
            EXPECT_EQ(s.sty.fg, hl.get_theme().number.fg);
        }
    }
    EXPECT_TRUE(found) << "Should detect integer 42";
}

TEST(ReprHighlighter, DetectsFloats) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("pi is 3.14");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "pi is 3.14";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "3.14") found = true;
    }
    EXPECT_TRUE(found) << "Should detect float 3.14";
}

TEST(ReprHighlighter, DetectsHex) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("address 0xFF00");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "address 0xFF00";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "0xFF00") {
            found = true;
            EXPECT_EQ(s.sty.fg, hl.get_theme().number_hex.fg);
        }
    }
    EXPECT_TRUE(found) << "Should detect hex 0xFF00";
}

TEST(ReprHighlighter, DetectsUrl) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("visit https://example.com/path today");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "visit https://example.com/path today";
        auto matched = text.substr(s.offset, s.length);
        if (matched.find("https://example.com") != std::string_view::npos) {
            found = true;
            EXPECT_EQ(s.sty.fg, hl.get_theme().url.fg);
            EXPECT_TRUE(has_attr(s.sty.attrs, attr::underline));
        }
    }
    EXPECT_TRUE(found) << "Should detect URL";
}

TEST(ReprHighlighter, DetectsIPv4) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("server at 192.168.1.1 port 80");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "server at 192.168.1.1 port 80";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "192.168.1.1") {
            found = true;
            EXPECT_EQ(s.sty.fg, hl.get_theme().ipv4.fg);
        }
    }
    EXPECT_TRUE(found) << "Should detect IPv4 address";
}

TEST(ReprHighlighter, DetectsUUID) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("id: 550e8400-e29b-41d4-a716-446655440000");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "id: 550e8400-e29b-41d4-a716-446655440000";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "550e8400-e29b-41d4-a716-446655440000") {
            found = true;
            EXPECT_EQ(s.sty.fg, hl.get_theme().uuid.fg);
        }
    }
    EXPECT_TRUE(found) << "Should detect UUID";
}

TEST(ReprHighlighter, DetectsBooleans) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("enabled: true disabled: false");
    int bool_count = 0;
    for (const auto &s : spans) {
        std::string_view text = "enabled: true disabled: false";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "true" || matched == "false") {
            ++bool_count;
            EXPECT_EQ(s.sty.fg, hl.get_theme().boolean.fg);
        }
    }
    EXPECT_GE(bool_count, 2) << "Should detect both true and false";
}

TEST(ReprHighlighter, DetectsNull) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("value is null or nullptr");
    int null_count = 0;
    for (const auto &s : spans) {
        std::string_view text = "value is null or nullptr";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "null" || matched == "nullptr") {
            ++null_count;
        }
    }
    EXPECT_GE(null_count, 2) << "Should detect null and nullptr";
}

TEST(ReprHighlighter, DetectsStrings) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight(R"(name is "hello world" done)");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = R"(name is "hello world" done)";
        auto matched = text.substr(s.offset, s.length);
        if (matched == R"("hello world")") {
            found = true;
            EXPECT_EQ(s.sty.fg, hl.get_theme().string.fg);
        }
    }
    EXPECT_TRUE(found) << "Should detect double-quoted string";
}

TEST(ReprHighlighter, DetectsEmail) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("contact user@example.com for help");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "contact user@example.com for help";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "user@example.com") {
            found = true;
            EXPECT_EQ(s.sty.fg, hl.get_theme().email.fg);
        }
    }
    EXPECT_TRUE(found) << "Should detect email address";
}

TEST(ReprHighlighter, DetectsKeywords) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("TODO fix this FIXME later");
    int kw_count = 0;
    for (const auto &s : spans) {
        std::string_view text = "TODO fix this FIXME later";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "TODO" || matched == "FIXME") ++kw_count;
    }
    EXPECT_GE(kw_count, 2) << "Should detect TODO and FIXME";
}

TEST(ReprHighlighter, DetectsEnvVars) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("path is $HOME or ${PATH}");
    int env_count = 0;
    for (const auto &s : spans) {
        std::string_view text = "path is $HOME or ${PATH}";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "$HOME" || matched == "${PATH}") ++env_count;
    }
    EXPECT_GE(env_count, 2) << "Should detect $HOME and ${PATH}";
}

TEST(ReprHighlighter, DetectsISODate) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("date: 2024-01-15 done");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "date: 2024-01-15 done";
        auto matched = text.substr(s.offset, s.length);
        if (matched.find("2024-01-15") != std::string_view::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Should detect ISO date";
}

TEST(ReprHighlighter, CustomTheme) {
    repr_highlighter::theme t;
    t.number = style{colors::magenta};
    repr_highlighter hl(t);

    auto spans = hl.highlight("value 42");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "value 42";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "42") {
            found = true;
            EXPECT_EQ(s.sty.fg, colors::magenta);
        }
    }
    EXPECT_TRUE(found);
}

TEST(ReprHighlighter, Singleton) {
    const auto &a = repr_highlighter::instance();
    const auto &b = repr_highlighter::instance();
    EXPECT_EQ(&a, &b);
}

// ═══════════════════════════════════════════════════════════════════════
// highlight_chain
// ═══════════════════════════════════════════════════════════════════════

TEST(HighlightChain, CombinesMultiple) {
    regex_highlighter hl1;
    hl1.add_rule(R"(\d+)", style{colors::cyan});

    regex_highlighter hl2;
    hl2.add_word("TODO", style{colors::yellow});

    highlight_chain chain;
    chain.add(hl1);
    chain.add(hl2);

    auto spans = chain.highlight("TODO: fix 42 bugs");
    // Should have spans from both highlighters
    bool found_num = false;
    bool found_kw = false;
    for (const auto &s : spans) {
        std::string_view text = "TODO: fix 42 bugs";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "42") found_num = true;
        if (matched == "TODO") found_kw = true;
    }
    EXPECT_TRUE(found_num);
    EXPECT_TRUE(found_kw);
}

TEST(HighlightChain, SharedOwnership) {
    auto hl = std::make_shared<regex_highlighter>();
    hl->add_rule(R"(\d+)", style{colors::cyan});

    highlight_chain chain;
    chain.add(hl);

    auto spans = chain.highlight("test 42");
    ASSERT_GE(spans.size(), 1u);
}

// ═══════════════════════════════════════════════════════════════════════
// highlight_text convenience
// ═══════════════════════════════════════════════════════════════════════

TEST(HighlightText, EndToEnd) {
    const auto &hl = repr_highlighter::instance();
    auto frags = highlight_text("visit https://example.com port 8080", hl);

    // Should have multiple fragments, some styled
    EXPECT_GE(frags.size(), 2u);

    bool has_styled = false;
    for (const auto &f : frags) {
        if (!f.sty.is_default()) has_styled = true;
    }
    EXPECT_TRUE(has_styled) << "Should have at least one styled fragment";
}

TEST(HighlightText, EmptyInput) {
    const auto &hl = repr_highlighter::instance();
    auto frags = highlight_text("", hl);
    EXPECT_TRUE(frags.empty());
}

TEST(HighlightText, NoMatches) {
    regex_highlighter hl;
    hl.add_rule(R"(ZZZZZ)", style{colors::red});

    auto frags = highlight_text("hello world", hl);
    ASSERT_EQ(frags.size(), 1u);
    EXPECT_EQ(frags[0].text, "hello world");
    EXPECT_TRUE(frags[0].sty.is_default());
}

// ═══════════════════════════════════════════════════════════════════════
// Console integration
// ═══════════════════════════════════════════════════════════════════════

TEST(ConsoleHighlight, SetHighlighter) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    EXPECT_EQ(con.get_highlighter(), nullptr);
    EXPECT_FALSE(con.auto_highlight());

    const auto &hl = repr_highlighter::instance();
    con.set_highlighter(&hl);
    EXPECT_EQ(con.get_highlighter(), &hl);

    con.set_auto_highlight(true);
    EXPECT_TRUE(con.auto_highlight());
}

TEST(ConsoleHighlight, PrintHighlighted) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_highlighted("count is 42 at https://example.com");
    // Should contain ANSI escape sequences
    EXPECT_NE(output.find("\033["), std::string::npos) << "Should contain ANSI sequences";
    // Should contain the text
    EXPECT_NE(output.find("count"), std::string::npos);
    EXPECT_NE(output.find("42"), std::string::npos);
}

TEST(ConsoleHighlight, PrintHighlightedNoHighlighter) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    // No highlighter set — should just output plain text
    con.print_highlighted("count is 42");
    EXPECT_NE(output.find("count is 42"), std::string::npos);
}

TEST(ConsoleHighlight, AutoHighlightOnPrint) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());
    con.set_auto_highlight(true);

    con.print("plain text with 42 and https://example.com");
    // Should contain ANSI sequences from auto-highlighting
    EXPECT_NE(output.find("\033["), std::string::npos) << "Auto-highlight should produce ANSI sequences";
}

TEST(ConsoleHighlight, MarkupOverridesAutoHighlight) {
    std::string output_with_markup;
    std::string output_plain;

    auto make_console = [](std::string &out) {
        console_config cfg;
        cfg.sink = [&out](std::string_view s) { out += s; };
        cfg.force_color = true;
        console con(cfg);
        con.set_highlighter(&repr_highlighter::instance());
        con.set_auto_highlight(true);
        return con;
    };

    auto con1 = make_console(output_with_markup);
    con1.print("[bold red]ERROR:[/] code 42");

    auto con2 = make_console(output_plain);
    con2.print("ERROR: code 42");

    // Both should have ANSI, but the markup version should have
    // explicit bold+red for "ERROR:" while auto-highlight handles "42"
    EXPECT_NE(output_with_markup.find("\033["), std::string::npos);
    EXPECT_NE(output_plain.find("\033["), std::string::npos);
    // They should be different because markup adds explicit styling
    EXPECT_NE(output_with_markup, output_plain);
}

// ═══════════════════════════════════════════════════════════════════════
// Edge cases
// ═══════════════════════════════════════════════════════════════════════

TEST(ReprHighlighter, StringsProtectContent) {
    const auto &hl = repr_highlighter::instance();
    // Numbers inside strings should be matched as strings, not numbers
    // (strings are matched first and take priority via merge_spans)
    auto frags = highlight_text(R"(msg "error 404" done)", hl);

    // The "error 404" should be one styled fragment (string style)
    bool found_string = false;
    for (const auto &f : frags) {
        if (f.text.find("error 404") != std::string::npos) {
            found_string = true;
            EXPECT_EQ(f.sty.fg, hl.get_theme().string.fg) << "Content inside quotes should get string style";
        }
    }
    EXPECT_TRUE(found_string);
}

TEST(ReprHighlighter, BinaryNumber) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("mask 0b1010");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "mask 0b1010";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "0b1010") found = true;
    }
    EXPECT_TRUE(found) << "Should detect binary number";
}

TEST(ReprHighlighter, OctalNumber) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("perms 0o755");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "perms 0o755";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "0o755") found = true;
    }
    EXPECT_TRUE(found) << "Should detect octal number";
}

TEST(ReprHighlighter, ScientificNotation) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("speed 3e8");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "speed 3e8";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "3e8") found = true;
    }
    EXPECT_TRUE(found) << "Should detect scientific notation";
}

TEST(ReprHighlighter, WindowsEnvVar) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("dir is %USERPROFILE%");
    bool found = false;
    for (const auto &s : spans) {
        std::string_view text = "dir is %USERPROFILE%";
        auto matched = text.substr(s.offset, s.length);
        if (matched == "%USERPROFILE%") found = true;
    }
    EXPECT_TRUE(found) << "Should detect Windows env var";
}

// ═══════════════════════════════════════════════════════════════════════
// OSC 8 hyperlink support
// ═══════════════════════════════════════════════════════════════════════

TEST(HyperlinkSpan, UrlHasLink) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("visit https://example.com today");
    bool found = false;
    for (const auto &s : spans) {
        if (s.link.find("https://example.com") != std::string::npos) {
            found = true;
            EXPECT_EQ(s.link, "https://example.com");
        }
    }
    EXPECT_TRUE(found) << "URL span should carry link URL";
}

TEST(HyperlinkSpan, UnixPathHasFileUri) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("path /usr/local/bin/app done");
    bool found = false;
    for (const auto &s : spans) {
        if (!s.link.empty()) {
            found = true;
            EXPECT_NE(s.link.find("file:///"), std::string::npos)
                << "Path link should be a file:// URI, got: " << s.link;
            EXPECT_NE(s.link.find("/usr/local/bin/app"), std::string::npos);
        }
    }
    EXPECT_TRUE(found) << "Path span should carry file:// link";
}

TEST(HyperlinkSpan, WindowsPathHasFileUri) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight(R"(dir C:\Windows\System32 end)");
    bool found = false;
    for (const auto &s : spans) {
        if (!s.link.empty()) {
            found = true;
            EXPECT_NE(s.link.find("file:///"), std::string::npos) << "Path link should be a file:// URI";
            // Backslashes should be converted to forward slashes
            EXPECT_EQ(s.link.find('\\'), std::string::npos) << "file:// URI should not contain backslashes";
        }
    }
    EXPECT_TRUE(found) << "Windows path span should carry file:// link";
}

TEST(HyperlinkSpan, NonLinkSpansHaveEmptyLink) {
    const auto &hl = repr_highlighter::instance();
    auto spans = hl.highlight("count 42 and true");
    for (const auto &s : spans) {
        EXPECT_TRUE(s.link.empty()) << "Numbers and booleans should not have links";
    }
}

TEST(LinkedFragment, UrlPreservesLink) {
    const auto &hl = repr_highlighter::instance();
    auto frags = highlight_text_linked("see https://example.com here", hl);
    bool found = false;
    for (const auto &f : frags) {
        if (!f.link.empty()) {
            found = true;
            EXPECT_EQ(f.link, "https://example.com");
            EXPECT_NE(f.text.find("https://example.com"), std::string::npos);
        }
    }
    EXPECT_TRUE(found) << "linked_fragment should carry URL";
}

TEST(LinkedFragment, PlainTextHasNoLink) {
    const auto &hl = repr_highlighter::instance();
    auto frags = highlight_text_linked("just plain text", hl);
    for (const auto &f : frags) {
        EXPECT_TRUE(f.link.empty());
    }
}

TEST(ConsoleHyperlink, PrintHighlightedEmitsOSC8) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_highlighted("visit https://example.com today");
    // Should contain OSC 8 open sequence with the URL
    EXPECT_NE(output.find("\033]8;;https://example.com\033\\"), std::string::npos)
        << "Should emit OSC 8 hyperlink open for URL";
    // Should contain OSC 8 close sequence
    EXPECT_NE(output.find("\033]8;;\033\\"), std::string::npos) << "Should emit OSC 8 hyperlink close";
}

TEST(ConsoleHyperlink, AutoHighlightEmitsOSC8) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());
    con.set_auto_highlight(true);

    con.print("check https://github.com/repo and 42");
    EXPECT_NE(output.find("\033]8;;https://github.com/repo\033\\"), std::string::npos)
        << "Auto-highlight print() should emit OSC 8 for URLs";
}

TEST(ConsoleHyperlink, PathEmitsFileUri) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_highlighted("path /usr/local/bin/app done");
    EXPECT_NE(output.find("\033]8;;file:///"), std::string::npos) << "Path should emit OSC 8 with file:// URI";
}

TEST(ConsoleHyperlink, NoLinkForPlainNumbers) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_highlighted("count 42");
    // Should NOT contain OSC 8 open (numbers aren't links)
    EXPECT_EQ(output.find("\033]8;;"), std::string::npos) << "Numbers should not produce OSC 8 hyperlinks";
}

// ═══════════════════════════════════════════════════════════════════════
// std::format-style print overloads
// ═══════════════════════════════════════════════════════════════════════

TEST(ConsoleFormat, PrintFormat) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.print("[bold]count:[/] {}", 42);
    EXPECT_NE(output.find("42"), std::string::npos);
    // Should end with newline
    EXPECT_EQ(output.back(), '\n');
}

TEST(ConsoleFormat, PrintFormatMultipleArgs) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.print("host {} port {}", "localhost", 8080);
    EXPECT_NE(output.find("localhost"), std::string::npos);
    EXPECT_NE(output.find("8080"), std::string::npos);
}

TEST(ConsoleFormat, PrintFormatWithMarkup) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.print("[bold red]Error {}:[/] {}", 404, "not found");
    // Should contain ANSI sequences from markup
    EXPECT_NE(output.find("\033["), std::string::npos);
    EXPECT_NE(output.find("404"), std::string::npos);
    EXPECT_NE(output.find("not found"), std::string::npos);
}

TEST(ConsoleFormat, PrintInlineFormat) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.print_inline("value={}", 3.14);
    EXPECT_NE(output.find("3.14"), std::string::npos);
    // Should NOT end with newline
    EXPECT_NE(output.back(), '\n');
}

TEST(ConsoleFormat, PrintHighlightedFormat) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_highlighted("count is {} at {}", 42, "https://example.com");
    // Should contain ANSI escape sequences from highlighting
    EXPECT_NE(output.find("\033["), std::string::npos);
    EXPECT_NE(output.find("42"), std::string::npos);
    EXPECT_NE(output.find("https://example.com"), std::string::npos);
}

TEST(ConsoleFormat, WriteFormat) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.write("x={} y={}", 10, 20);
    EXPECT_EQ(output, "x=10 y=20");
}

TEST(ConsoleFormat, PrintFormatStringType) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    std::string name = "tapiru";
    con.print("name: {}", name);
    EXPECT_NE(output.find("tapiru"), std::string::npos);
}

TEST(ConsoleFormat, PrintFormatNoArgs) {
    // Calling print with a plain string (no {}) should still work
    // via the string_view overload, not the format overload
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.print("hello world");
    EXPECT_NE(output.find("hello world"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════════
// print_hl / print_hl_inline aliases
// ═══════════════════════════════════════════════════════════════════════

TEST(ConsoleAlias, PrintHl) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_hl("count 42");
    EXPECT_NE(output.find("42"), std::string::npos);
    EXPECT_NE(output.find("\033["), std::string::npos) << "print_hl should produce ANSI sequences";
    EXPECT_EQ(output.back(), '\n');
}

TEST(ConsoleAlias, PrintHlFormat) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_hl("value is {}", 99);
    EXPECT_NE(output.find("99"), std::string::npos);
}

TEST(ConsoleAlias, PrintHlInline) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_hl_inline("count 42");
    EXPECT_NE(output.find("42"), std::string::npos);
    EXPECT_NE(output.back(), '\n') << "print_hl_inline should not append newline";
}

TEST(ConsoleAlias, PrintHlInlineFormat) {
    std::string output;
    console_config cfg;
    cfg.sink = [&](std::string_view s) { output += s; };
    cfg.force_color = true;

    console con(cfg);
    con.set_highlighter(&repr_highlighter::instance());

    con.print_hl_inline("x={}", 3.14);
    EXPECT_NE(output.find("3.14"), std::string::npos);
    EXPECT_NE(output.back(), '\n');
}

// ═══════════════════════════════════════════════════════════════════════
// Global tcon
// ═══════════════════════════════════════════════════════════════════════

TEST(GlobalTcon, Exists) {
    // tcon is a global tapiru::console — verify it's usable
    tcon.write(""); // should not crash
}

TEST(GlobalTcon, PrintWorks) {
    // tcon defaults to stdout; just verify it doesn't crash
    tcon.print("");
}
