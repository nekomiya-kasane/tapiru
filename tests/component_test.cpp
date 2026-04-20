/**
 * @file component_test.cpp
 * @brief Tests for component_base, make_renderer, containers, catch_event.
 */

#include "tapiru/core/component.h"
#include "tapiru/core/console.h"
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

static std::string render_component(component c, uint32_t width = 40) {
    virtual_terminal vt;
    auto con = vt.make_console();
    auto elem = c->render();
    con.print_widget(elem, width);
    return vt.raw();
}

// ── make_renderer ───────────────────────────────────────────────────────

TEST(ComponentTest, RendererProducesElement) {
    auto r = make_renderer([](bool) { return element(text_builder("hello")); });
    auto out = render_component(r);
    EXPECT_TRUE(out.find("hello") != std::string::npos);
}

TEST(ComponentTest, SimpleRendererProducesElement) {
    auto r = make_renderer([]() { return element(text_builder("simple")); });
    auto out = render_component(r);
    EXPECT_TRUE(out.find("simple") != std::string::npos);
}

TEST(ComponentTest, RendererReceivesFocusState) {
    bool received_focus = false;
    auto r = make_renderer([&](bool focused) -> element {
        received_focus = focused;
        return element(text_builder("x"));
    });
    r->set_focused(true);
    r->render();
    EXPECT_TRUE(received_focus);

    r->set_focused(false);
    r->render();
    EXPECT_FALSE(received_focus);
}

TEST(ComponentTest, RendererNotFocusable) {
    auto r = make_renderer([]() { return element(text_builder("x")); });
    EXPECT_FALSE(r->focusable());
}

// ── container::vertical ─────────────────────────────────────────────────

TEST(ComponentTest, VerticalContainerRendersChildren) {
    auto c1 = make_renderer([]() { return element(text_builder("AAA")); });
    auto c2 = make_renderer([]() { return element(text_builder("BBB")); });
    auto v = container::vertical({c1, c2});
    auto out = render_component(v);
    EXPECT_TRUE(out.find("AAA") != std::string::npos);
    EXPECT_TRUE(out.find("BBB") != std::string::npos);
}

TEST(ComponentTest, HorizontalContainerRendersChildren) {
    auto c1 = make_renderer([]() { return element(text_builder("LLL")); });
    auto c2 = make_renderer([]() { return element(text_builder("RRR")); });
    auto h = container::horizontal({c1, c2});
    auto out = render_component(h);
    EXPECT_TRUE(out.find("LLL") != std::string::npos);
    EXPECT_TRUE(out.find("RRR") != std::string::npos);
}

TEST(ComponentTest, EmptyContainerDoesNotCrash) {
    auto v = container::vertical({});
    auto out = render_component(v);
    // Should produce some output (empty rows) without crashing
    EXPECT_TRUE(true);
}

// ── container focus cycling ─────────────────────────────────────────────

namespace {
class focusable_component : public component_base {
  public:
    explicit focusable_component(std::string label) : label_(std::move(label)) {}
    element render() override { return element(text_builder(label_)); }
    bool focusable() const override { return true; }
    bool on_event(const input_event &ev) override {
        if (auto *ke = std::get_if<key_event>(&ev)) {
            if (ke->key == special_key::enter) {
                activated_ = true;
                return true;
            }
        }
        return false;
    }
    bool activated_ = false;

  private:
    std::string label_;
};
} // anonymous namespace

TEST(ComponentTest, ContainerFocusCycleTab) {
    auto f1 = std::make_shared<focusable_component>("F1");
    auto f2 = std::make_shared<focusable_component>("F2");
    auto f3 = std::make_shared<focusable_component>("F3");
    auto v = container::vertical({f1, f2, f3});

    // Initially first focusable child should be active
    auto active = v->active_child();
    EXPECT_EQ(active.get(), f1.get());

    // Tab to next
    key_event tab_ev{0, special_key::tab, key_mod::none};
    EXPECT_TRUE(v->on_event(tab_ev));
    EXPECT_EQ(v->active_child().get(), f2.get());

    // Tab again
    EXPECT_TRUE(v->on_event(tab_ev));
    EXPECT_EQ(v->active_child().get(), f3.get());

    // Tab wraps around
    EXPECT_TRUE(v->on_event(tab_ev));
    EXPECT_EQ(v->active_child().get(), f1.get());
}

TEST(ComponentTest, ContainerFocusCycleShiftTab) {
    auto f1 = std::make_shared<focusable_component>("F1");
    auto f2 = std::make_shared<focusable_component>("F2");
    auto v = container::vertical({f1, f2});

    key_event shift_tab{0, special_key::tab, key_mod::shift};
    EXPECT_TRUE(v->on_event(shift_tab));
    EXPECT_EQ(v->active_child().get(), f2.get());
}

TEST(ComponentTest, ContainerDispatchesToActiveChild) {
    auto f1 = std::make_shared<focusable_component>("F1");
    auto f2 = std::make_shared<focusable_component>("F2");
    auto v = container::vertical({f1, f2});

    // Enter should be dispatched to f1 (active child)
    key_event enter_ev{0, special_key::enter, key_mod::none};
    EXPECT_TRUE(v->on_event(enter_ev));
    EXPECT_TRUE(f1->activated_);
    EXPECT_FALSE(f2->activated_);
}

// ── tab container ───────────────────────────────────────────────────────

TEST(ComponentTest, TabContainerShowsSelected) {
    int sel = 0;
    auto c1 = make_renderer([]() { return element(text_builder("TAB0")); });
    auto c2 = make_renderer([]() { return element(text_builder("TAB1")); });
    auto t = container::tab({c1, c2}, &sel);

    auto out0 = render_component(t);
    EXPECT_TRUE(out0.find("TAB0") != std::string::npos);

    sel = 1;
    auto out1 = render_component(t);
    EXPECT_TRUE(out1.find("TAB1") != std::string::npos);
}

// ── stacked container ───────────────────────────────────────────────────

TEST(ComponentTest, StackedContainerShowsSelected) {
    int sel = 1;
    auto c1 = make_renderer([]() { return element(text_builder("STACK0")); });
    auto c2 = make_renderer([]() { return element(text_builder("STACK1")); });
    auto s = container::stacked({c1, c2}, &sel);

    auto out = render_component(s);
    EXPECT_TRUE(out.find("STACK1") != std::string::npos);
}

// ── catch_event ─────────────────────────────────────────────────────────

TEST(ComponentTest, CatchEventInterceptsEvent) {
    bool intercepted = false;
    auto inner = make_renderer([]() { return element(text_builder("x")); });
    auto wrapped = catch_event(inner, [&](const input_event &ev) {
        if (auto *ke = std::get_if<key_event>(&ev)) {
            if (ke->key == special_key::escape) {
                intercepted = true;
                return true;
            }
        }
        return false;
    });

    key_event esc{0, special_key::escape, key_mod::none};
    EXPECT_TRUE(wrapped->on_event(esc));
    EXPECT_TRUE(intercepted);
}

TEST(ComponentTest, CatchEventPassesThrough) {
    auto inner = make_renderer([]() { return element(text_builder("x")); });
    auto wrapped = catch_event(inner, [](const input_event &) { return false; });

    key_event enter_ev{0, special_key::enter, key_mod::none};
    EXPECT_FALSE(wrapped->on_event(enter_ev));
}

// ── component children ──────────────────────────────────────────────────

TEST(ComponentTest, ContainerReturnsChildren) {
    auto c1 = make_renderer([]() { return element(text_builder("x")); });
    auto c2 = make_renderer([]() { return element(text_builder("y")); });
    auto v = container::vertical({c1, c2});
    auto kids = v->children();
    EXPECT_EQ(kids.size(), 2u);
}

TEST(ComponentTest, RendererHasNoChildren) {
    auto r = make_renderer([]() { return element(text_builder("x")); });
    EXPECT_TRUE(r->children().empty());
}
