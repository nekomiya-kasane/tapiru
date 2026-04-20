/**
 * @file element_test.cpp
 * @brief Tests for element type-erasure and decorator pipe composition.
 */

#include <gtest/gtest.h>

#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/core/console.h"
#include "tapiru/widgets/builders.h"

using namespace tapiru;

// ── VirtualTerminal helper ──────────────────────────────────────────────

class virtual_terminal {
public:
    [[nodiscard]] console make_console(bool color = false) {
        console_config cfg;
        cfg.sink = [this](std::string_view data) { buffer_ += data; };
        cfg.depth = color ? color_depth::true_color : color_depth::none;
        cfg.force_color = color;
        cfg.no_color = !color;
        return console(cfg);
    }
    [[nodiscard]] const std::string& raw() const noexcept { return buffer_; }
    void clear() { buffer_.clear(); }
private:
    std::string buffer_;
};

static std::string render(auto&& builder, uint32_t width = 40) {
    virtual_terminal vt;
    auto con = vt.make_console(false);
    con.print_widget(std::forward<decltype(builder)>(builder), width);
    return vt.raw();
}

// ── element from builder ────────────────────────────────────────────────

TEST(ElementTest, ElementFromTextBuilder) {
    element e = text_builder("hello");
    EXPECT_FALSE(e.empty());
    auto out = render(e);
    EXPECT_TRUE(out.find("hello") != std::string::npos);
}

TEST(ElementTest, ElementFromPanelBuilder) {
    element e = panel_builder(text_builder("inside"));
    EXPECT_FALSE(e.empty());
    auto out = render(e, 30);
    EXPECT_TRUE(out.find("inside") != std::string::npos);
}

TEST(ElementTest, EmptyElement) {
    element e;
    EXPECT_TRUE(e.empty());
}

TEST(ElementTest, ElementFlattenProducesOutput) {
    element e = text_builder("test");
    auto out = render(e);
    EXPECT_FALSE(out.empty());
}

// ── decorator pipe ──────────────────────────────────────────────────────

TEST(ElementTest, DecoratorBorderWraps) {
    element e = text_builder("boxed");
    element bordered = e | border();
    auto out = render(bordered, 30);
    // Should contain the text and border characters
    EXPECT_TRUE(out.find("boxed") != std::string::npos);
    EXPECT_GT(out.size(), std::string("boxed").size());
}

TEST(ElementTest, DecoratorPaddingAddsSpace) {
    element base = text_builder("pad");
    auto out_no_pad = render(base, 20);

    element padded = text_builder("pad") | padding(uint8_t{1});
    auto out_padded = render(padded, 20);

    // Padded output should be larger (more whitespace)
    EXPECT_GE(out_padded.size(), out_no_pad.size());
}

TEST(ElementTest, DecoratorChaining) {
    element e = text_builder("chain")
        | border()
        | padding(uint8_t{1})
        | bold();
    EXPECT_FALSE(e.empty());
    auto out = render(e, 40);
    EXPECT_TRUE(out.find("chain") != std::string::npos);
}

TEST(ElementTest, DecoratorFlex) {
    // flex() should not crash and should pass through the element
    element e = text_builder("flex") | flex();
    EXPECT_FALSE(e.empty());
    auto out = render(e);
    EXPECT_TRUE(out.find("flex") != std::string::npos);
}

TEST(ElementTest, DecoratorMaybe) {
    bool show = true;
    element e1 = text_builder("visible") | maybe(&show);
    auto out1 = render(e1);
    EXPECT_TRUE(out1.find("visible") != std::string::npos);

    show = false;
    element e2 = text_builder("hidden") | maybe(&show);
    auto out2 = render(e2);
    // When hidden, the element should be empty or produce no visible text
    EXPECT_TRUE(out2.find("hidden") == std::string::npos);
}

TEST(ElementTest, DecoratorStyleShortcut) {
    style s;
    s.attrs = attr::bold;
    element e = text_builder("styled") | s;
    EXPECT_FALSE(e.empty());
    auto out = render(e);
    EXPECT_TRUE(out.find("styled") != std::string::npos);
}

TEST(ElementTest, DecoratorCenter) {
    element e = text_builder("mid") | center();
    EXPECT_FALSE(e.empty());
    auto out = render(e, 40);
    EXPECT_TRUE(out.find("mid") != std::string::npos);
}
