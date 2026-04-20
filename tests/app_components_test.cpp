/**
 * @file app_components_test.cpp
 * @brief Tests for app-level component factories: menu_bar, status_bar, resizable_split.
 */

#include "tapiru/core/console.h"
#include "tapiru/widgets/app_components.h"
#include "tapiru/widgets/builders.h"

#include <gtest/gtest.h>

using namespace tapiru;

// ── Helper ──────────────────────────────────────────────────────────────

class virtual_terminal {
  public:
    [[nodiscard]] console make_console() {
        console_config cfg;
        cfg.sink = [this](std::string_view data) { buffer_ += data; };
        cfg.depth = color_depth::none;
        cfg.no_color = true;
        return console(cfg);
    }
    [[nodiscard]] const std::string &raw() const noexcept { return buffer_; }
    void clear() { buffer_.clear(); }

  private:
    std::string buffer_;
};

static std::string render_comp(component c, uint32_t width = 60) {
    virtual_terminal vt;
    auto con = vt.make_console();
    auto elem = c->render();
    con.print_widget(elem, width);
    return vt.raw();
}

// ── menu_bar ────────────────────────────────────────────────────────────

TEST(AppComponentTest, MenuBarRendersLabels) {
    int sel = 0;
    std::vector<menu_bar_entry> entries = {
        {"File", {}},
        {"Edit", {}},
        {"View", {}},
    };
    auto mb = make_menu_bar(entries, &sel);
    auto out = render_comp(mb);
    EXPECT_TRUE(out.find("File") != std::string::npos);
    EXPECT_TRUE(out.find("Edit") != std::string::npos);
    EXPECT_TRUE(out.find("View") != std::string::npos);
}

TEST(AppComponentTest, MenuBarIsFocusable) {
    int sel = 0;
    auto mb = make_menu_bar({{"File", {}}}, &sel);
    EXPECT_TRUE(mb->focusable());
}

TEST(AppComponentTest, MenuBarKeyboardNav) {
    int sel = 0;
    auto mb = make_menu_bar({{"A", {}}, {"B", {}}, {"C", {}}}, &sel);

    key_event right{0, special_key::right, key_mod::none};
    EXPECT_TRUE(mb->on_event(right));
    EXPECT_EQ(sel, 1);

    EXPECT_TRUE(mb->on_event(right));
    EXPECT_EQ(sel, 2);

    // Wrap around
    EXPECT_TRUE(mb->on_event(right));
    EXPECT_EQ(sel, 0);
}

TEST(AppComponentTest, MenuBarLeftNav) {
    int sel = 0;
    auto mb = make_menu_bar({{"A", {}}, {"B", {}}}, &sel);

    key_event left{0, special_key::left, key_mod::none};
    EXPECT_TRUE(mb->on_event(left));
    EXPECT_EQ(sel, 1); // wraps to last
}

TEST(AppComponentTest, MenuBarOpenClose) {
    int sel = 0;
    auto mb = make_menu_bar({{"File", {}}}, &sel);

    key_event enter{0, special_key::enter, key_mod::none};
    EXPECT_TRUE(mb->on_event(enter)); // open

    key_event esc{0, special_key::escape, key_mod::none};
    EXPECT_TRUE(mb->on_event(esc)); // close
}

// ── status_bar ──────────────────────────────────────────────────────────

TEST(AppComponentTest, StatusBarRendersContent) {
    auto sb = make_status_bar([]() { return element(text_builder("Ready | Ln 1, Col 1")); });
    auto out = render_comp(sb);
    EXPECT_TRUE(out.find("Ready") != std::string::npos);
}

TEST(AppComponentTest, StatusBarNotFocusable) {
    auto sb = make_status_bar([]() { return element(text_builder("x")); });
    EXPECT_FALSE(sb->focusable());
}

// ── resizable_split ─────────────────────────────────────────────────────

TEST(AppComponentTest, SplitLeftRender) {
    auto left = make_renderer([]() { return element(text_builder("LEFT")); });
    auto right = make_renderer([]() { return element(text_builder("RIGHT")); });
    int split = 30;
    auto sp = resizable_split_left(left, right, &split);
    auto out = render_comp(sp, 60);
    EXPECT_TRUE(out.find("LEFT") != std::string::npos);
    EXPECT_TRUE(out.find("RIGHT") != std::string::npos);
}

TEST(AppComponentTest, SplitTopRender) {
    auto top = make_renderer([]() { return element(text_builder("TOP")); });
    auto bottom = make_renderer([]() { return element(text_builder("BOTTOM")); });
    int split = 5;
    auto sp = resizable_split_top(top, bottom, &split);
    auto out = render_comp(sp, 40);
    EXPECT_TRUE(out.find("TOP") != std::string::npos);
    EXPECT_TRUE(out.find("BOTTOM") != std::string::npos);
}

TEST(AppComponentTest, SplitTabSwitchesFocus) {
    auto left = make_renderer([]() { return element(text_builder("L")); });
    auto right = make_renderer([]() { return element(text_builder("R")); });
    int split = 20;
    auto sp = resizable_split_left(left, right, &split);

    // Initially left is active
    EXPECT_EQ(sp->active_child().get(), left.get());

    // Tab switches to right
    key_event tab{0, special_key::tab, key_mod::none};
    EXPECT_TRUE(sp->on_event(tab));
    EXPECT_EQ(sp->active_child().get(), right.get());

    // Tab switches back to left
    EXPECT_TRUE(sp->on_event(tab));
    EXPECT_EQ(sp->active_child().get(), left.get());
}

TEST(AppComponentTest, SplitReturnsChildren) {
    auto left = make_renderer([]() { return element(text_builder("L")); });
    auto right = make_renderer([]() { return element(text_builder("R")); });
    int split = 20;
    auto sp = resizable_split_left(left, right, &split);
    auto kids = sp->children();
    EXPECT_EQ(kids.size(), 2u);
}

TEST(AppComponentTest, SplitIsFocusable) {
    auto left = make_renderer([]() { return element(text_builder("L")); });
    auto right = make_renderer([]() { return element(text_builder("R")); });
    int split = 20;
    auto sp = resizable_split_left(left, right, &split);
    EXPECT_TRUE(sp->focusable());
}

// ── composing app layout ────────────────────────────────────────────────

TEST(AppComponentTest, ComposedAppLayout) {
    int sel = 0;
    auto menu = make_menu_bar({{"File", {}}, {"Edit", {}}}, &sel);
    auto content = make_renderer([]() { return element(text_builder("Content area")); });
    auto status = make_status_bar([]() { return element(text_builder("Ready")); });

    auto layout = container::vertical({menu, content, status});
    auto out = render_comp(layout, 60);
    EXPECT_TRUE(out.find("File") != std::string::npos);
    EXPECT_TRUE(out.find("Content area") != std::string::npos);
    EXPECT_TRUE(out.find("Ready") != std::string::npos);
}
