/**
 * @file image_test.cpp
 * @brief Tests for image_builder half-block rendering.
 */

#include "tapiru/core/console.h"
#include "tapiru/widgets/image.h"

#include <gtest/gtest.h>

using namespace tapiru;

class virtual_terminal {
  public:
    [[nodiscard]] console make_console(bool color = false, uint32_t width = 80) {
        console_config cfg;
        cfg.sink = [this](std::string_view data) { buffer_ += data; };
        cfg.depth = color ? color_depth::true_color : color_depth::none;
        cfg.force_color = color;
        cfg.no_color = !color;
        return console(cfg);
    }
    [[nodiscard]] const std::string &raw() const noexcept { return buffer_; }
    void clear() { buffer_.clear(); }

  private:
    std::string buffer_;
};

TEST(ImageBuilderTest, EmptyPixelsRendersEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(true, 80);
    con.print_widget(image_builder({}, 0, 0).target_width(10), 80);
    // Should not crash
}

TEST(ImageBuilderTest, SinglePixelRenders) {
    virtual_terminal vt;
    auto con = vt.make_console(true, 80);
    std::vector<pixel_rgba> px = {{255, 0, 0, 255}};
    con.print_widget(image_builder(px, 1, 1).target_width(1), 80);
    auto &out = vt.raw();
    EXPECT_FALSE(out.empty());
}

TEST(ImageBuilderTest, RedGreenGradient) {
    // 4x2 image: top row red, bottom row green
    std::vector<pixel_rgba> px = {
        {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255}, {255, 0, 0, 255},
        {0, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 0, 255},
    };
    virtual_terminal vt;
    auto con = vt.make_console(true, 80);
    con.print_widget(image_builder(px, 4, 2).target_width(4), 80);
    auto &out = vt.raw();
    // Should contain ANSI color sequences
    EXPECT_TRUE(out.find("38;2;") != std::string::npos);
    EXPECT_TRUE(out.find("48;2;") != std::string::npos);
}

TEST(ImageBuilderTest, ContainsHalfBlockChar) {
    std::vector<pixel_rgba> px = {{128, 128, 128, 255}};
    virtual_terminal vt;
    auto con = vt.make_console(true, 80);
    con.print_widget(image_builder(px, 1, 1).target_width(1), 80);
    auto &out = vt.raw();
    // Half-block upper ▀ = E2 96 80
    EXPECT_TRUE(out.find("\xe2\x96\x80") != std::string::npos);
}

TEST(ImageBuilderTest, LargerImageScalesDown) {
    // 100x100 image, all blue
    std::vector<pixel_rgba> px(100 * 100, {0, 0, 255, 255});
    virtual_terminal vt;
    auto con = vt.make_console(true, 80);
    con.print_widget(image_builder(px, 100, 100).target_width(10), 80);
    auto &out = vt.raw();
    EXPECT_FALSE(out.empty());
    EXPECT_TRUE(out.find("0;0;255") != std::string::npos);
}

TEST(PixelRgbaTest, DefaultValues) {
    pixel_rgba p;
    EXPECT_EQ(p.r, 0);
    EXPECT_EQ(p.g, 0);
    EXPECT_EQ(p.b, 0);
    EXPECT_EQ(p.a, 255);
}
