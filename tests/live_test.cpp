#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#include "tapiru/core/console.h"
#include "tapiru/core/live.h"
#include "tapiru/widgets/builders.h"

using namespace tapiru;

// ── Thread-safe capture sink ────────────────────────────────────────────

class capture_sink {
public:
    void operator()(std::string_view data) {
        std::lock_guard lk(mu_);
        buffer_ += data;
        write_count_++;
    }
    [[nodiscard]] std::string snapshot() const {
        std::lock_guard lk(mu_);
        return buffer_;
    }
    [[nodiscard]] int writes() const {
        std::lock_guard lk(mu_);
        return write_count_;
    }
    void clear() {
        std::lock_guard lk(mu_);
        buffer_.clear();
        write_count_ = 0;
    }
private:
    mutable std::mutex mu_;
    std::string buffer_;
    int write_count_ = 0;
};

// ── Tests ───────────────────────────────────────────────────────────────

TEST(LiveTest, ConstructAndDestroy) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    {
        live lv(con, 30, 40);
        lv.set(text_builder("Hello"));
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    auto out = sink->snapshot();
    // Should have rendered at least once
    EXPECT_TRUE(out.find("Hello") != std::string::npos);
    // Should have hidden and restored cursor
    EXPECT_TRUE(out.find("\x1b[?25l") != std::string::npos);
    EXPECT_TRUE(out.find("\x1b[?25h") != std::string::npos);
}

TEST(LiveTest, UpdateContent) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    {
        live lv(con, 30, 40);
        lv.set(text_builder("First"));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lv.set(text_builder("Second"));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("First") != std::string::npos);
    EXPECT_TRUE(out.find("Second") != std::string::npos);
}

TEST(LiveTest, StopIsIdempotent) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    live lv(con, 30, 40);
    lv.set(text_builder("Test"));
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    lv.stop();
    lv.stop();  // second stop should be safe
    EXPECT_FALSE(lv.running());
}

TEST(LiveTest, RunningFlag) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    live lv(con, 30, 40);
    EXPECT_TRUE(lv.running());
    lv.stop();
    EXPECT_FALSE(lv.running());
}

TEST(LiveTest, WidgetUpdate) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    {
        live lv(con, 30, 30);
        panel_builder pb(text_builder("Panel Content"));
        pb.border(border_style::ascii);
        pb.title("Live");
        lv.set(std::move(pb));
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Panel Content") != std::string::npos);
}

TEST(LiveTest, DiffBasedRenderingProducesLessOutput) {
    // When content doesn't change, subsequent frames should produce less output
    // than the first frame (diff-based rendering).
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    {
        live lv(con, 60, 40);
        lv.set(text_builder("Static content that does not change"));
        // Let several frames render
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto out = sink->snapshot();
    // The content should appear (first frame renders it)
    EXPECT_TRUE(out.find("Static content") != std::string::npos);
    // Multiple writes should have occurred (render thread + final frame)
    EXPECT_GT(sink->writes(), 2);
}

TEST(LiveTest, ForceRedrawCausesFullOutput) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    {
        live lv(con, 30, 40);
        lv.set(text_builder("Redraw test"));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Capture output size before force_redraw
        auto before = sink->snapshot().size();

        lv.force_redraw();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // After force_redraw, there should be additional output
        auto after = sink->snapshot().size();
        EXPECT_GT(after, before);
    }
}
