#include <gtest/gtest.h>

#include <memory>
#include <mutex>
#include <string>

#include "tapiru/core/console.h"
#include "tapiru/widgets/spinner.h"

using namespace tapiru;

// ── spinner_state unit tests ────────────────────────────────────────────

TEST(SpinnerStateTest, InitialState) {
    spinner_state st;
    EXPECT_EQ(st.frame(), 0u);
    EXPECT_FALSE(st.done());
}

TEST(SpinnerStateTest, Tick) {
    spinner_state st;
    st.tick();
    EXPECT_EQ(st.frame(), 1u);
    st.tick();
    st.tick();
    EXPECT_EQ(st.frame(), 3u);
}

TEST(SpinnerStateTest, Done) {
    spinner_state st;
    st.set_done();
    EXPECT_TRUE(st.done());
}

// ── spinner_frames ──────────────────────────────────────────────────────

TEST(SpinnerFramesTest, Dots) {
    auto& f = spinner_frames::dots();
    EXPECT_EQ(f.size(), 10u);
}

TEST(SpinnerFramesTest, Line) {
    auto& f = spinner_frames::line();
    EXPECT_EQ(f.size(), 4u);
    EXPECT_EQ(f[0], "-");
    EXPECT_EQ(f[1], "\\");
    EXPECT_EQ(f[2], "|");
    EXPECT_EQ(f[3], "/");
}

TEST(SpinnerFramesTest, Arc) {
    auto& f = spinner_frames::arc();
    EXPECT_EQ(f.size(), 4u);
}

// ── spinner_builder rendering tests ─────────────────────────────────────

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
    void clear() {
        std::lock_guard lk(mu_);
        buffer_.clear();
    }
private:
    mutable std::mutex mu_;
    std::string buffer_;
};

TEST(SpinnerBuilderTest, RenderWithMessage) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    auto st = std::make_shared<spinner_state>();
    con.print_widget(
        spinner_builder(st)
            .message("Loading...")
            .frames({"-", "\\", "|", "/"}),
        40);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Loading...") != std::string::npos);
}

TEST(SpinnerBuilderTest, RenderDone) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    auto st = std::make_shared<spinner_state>();
    st->set_done();

    con.print_widget(
        spinner_builder(st)
            .message("Done!")
            .done_text("OK"),
        40);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("OK") != std::string::npos);
    EXPECT_TRUE(out.find("Done!") != std::string::npos);
}

TEST(SpinnerBuilderTest, FrameAdvances) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    auto st = std::make_shared<spinner_state>();

    // Render twice — frame should advance
    con.print_widget(spinner_builder(st).frames({"-", "|"}), 40);
    sink->clear();
    con.print_widget(spinner_builder(st).frames({"-", "|"}), 40);

    // After two renders, frame should be 2 (tick called once per flatten_into)
    EXPECT_GE(st->frame(), 2u);
}
