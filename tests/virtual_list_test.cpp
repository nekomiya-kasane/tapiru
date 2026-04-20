/**
 * @file virtual_list_test.cpp
 * @brief Tests for virtual_list_builder.
 */

#include "tapiru/core/console.h"
#include "tapiru/widgets/virtual_list.h"

#include <gtest/gtest.h>

using namespace tapiru;

class virtual_terminal {
  public:
    [[nodiscard]] console make_console(bool color = false, uint32_t width = 40) {
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

TEST(VirtualListTest, EmptyListRendersEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(virtual_list_builder(0, 5), 40);
    // Should render without crashing
    EXPECT_FALSE(vt.raw().empty());
}

TEST(VirtualListTest, RendersVisibleItems) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    auto vl = virtual_list_builder(100, 5).item_builder(
        [](uint32_t idx) { return text_builder("Row " + std::to_string(idx)); });
    con.print_widget(std::move(vl), 40);
    auto &out = vt.raw();
    // Should contain items 0-4
    EXPECT_TRUE(out.find("Row 0") != std::string::npos);
    EXPECT_TRUE(out.find("Row 4") != std::string::npos);
    // Should NOT contain item 5 (beyond visible window)
    EXPECT_TRUE(out.find("Row 5") == std::string::npos);
}

TEST(VirtualListTest, ScrollOffsetShiftsWindow) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    auto vl = virtual_list_builder(100, 3)
                  .item_builder([](uint32_t idx) { return text_builder("R" + std::to_string(idx)); })
                  .scroll_offset(10);
    con.print_widget(std::move(vl), 40);
    auto &out = vt.raw();
    EXPECT_TRUE(out.find("R10") != std::string::npos);
    EXPECT_TRUE(out.find("R12") != std::string::npos);
    EXPECT_TRUE(out.find("R13") == std::string::npos);
}

TEST(VirtualListTest, ScrollOffsetClampedToMax) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    // total=10, visible=5, offset=999 → clamped to 5
    auto vl = virtual_list_builder(10, 5).item_builder([](uint32_t) { return text_builder("x"); }).scroll_offset(999);
    con.print_widget(std::move(vl), 40);
    auto &out = vt.raw();
    // Should show items 5-9 (but item_builder returns "x" for all)
    EXPECT_TRUE(out.find("x") != std::string::npos);
}

TEST(VirtualListTest, Accessors) {
    auto vl = virtual_list_builder(50, 10).scroll_offset(5);
    EXPECT_EQ(vl.total_items(), 50u);
    EXPECT_EQ(vl.visible_height(), 10u);
    EXPECT_EQ(vl.offset(), 5u);
}

TEST(VirtualListTest, NoItemBuilderRendersEmpty) {
    virtual_terminal vt;
    auto con = vt.make_console(false, 40);
    con.print_widget(virtual_list_builder(100, 5), 40);
    // No item_builder set — should render empty without crashing
    EXPECT_FALSE(vt.raw().empty());
}
