#include <gtest/gtest.h>

#include <memory>
#include <mutex>
#include <string>

#include "tapiru/core/console.h"
#include "tapiru/widgets/progress.h"

using namespace tapiru;

// ── progress_task unit tests ────────────────────────────────────────────

TEST(ProgressTaskTest, InitialState) {
    progress_task task("Download", 100);
    EXPECT_EQ(task.description(), "Download");
    EXPECT_EQ(task.total(), 100u);
    EXPECT_EQ(task.current(), 0u);
    EXPECT_FALSE(task.completed());
    EXPECT_DOUBLE_EQ(task.fraction(), 0.0);
}

TEST(ProgressTaskTest, Advance) {
    progress_task task("Work", 200);
    task.advance(50);
    EXPECT_EQ(task.current(), 50u);
    task.advance(30);
    EXPECT_EQ(task.current(), 80u);
    EXPECT_NEAR(task.fraction(), 0.4, 0.001);
}

TEST(ProgressTaskTest, SetCurrent) {
    progress_task task("Work", 100);
    task.set_current(75);
    EXPECT_EQ(task.current(), 75u);
    EXPECT_NEAR(task.fraction(), 0.75, 0.001);
}

TEST(ProgressTaskTest, SetCompleted) {
    progress_task task("Work", 100);
    task.advance(50);
    task.set_completed();
    EXPECT_TRUE(task.completed());
    EXPECT_EQ(task.current(), 100u);
    EXPECT_DOUBLE_EQ(task.fraction(), 1.0);
}

TEST(ProgressTaskTest, SetTotal) {
    progress_task task("Work", 100);
    task.advance(50);
    task.set_total(200);
    EXPECT_EQ(task.total(), 200u);
    EXPECT_NEAR(task.fraction(), 0.25, 0.001);
}

TEST(ProgressTaskTest, FractionClamps) {
    progress_task task("Work", 100);
    task.set_current(150);  // over total
    EXPECT_DOUBLE_EQ(task.fraction(), 1.0);
}

TEST(ProgressTaskTest, ZeroTotal) {
    progress_task task("Empty", 0);
    EXPECT_DOUBLE_EQ(task.fraction(), 0.0);
}

// ── progress_builder rendering tests ────────────────────────────────────

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

TEST(ProgressBuilderTest, SingleTask) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    auto task = std::make_shared<progress_task>("Loading", 100);
    task->advance(50);

    con.print_widget(
        progress_builder().add_task(task).bar_width(20).complete_char('#').remaining_char('.'),
        80);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Loading") != std::string::npos);
    EXPECT_TRUE(out.find("50%") != std::string::npos);
    EXPECT_TRUE(out.find('#') != std::string::npos);
    EXPECT_TRUE(out.find('.') != std::string::npos);
}

TEST(ProgressBuilderTest, CompletedTask) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    auto task = std::make_shared<progress_task>("Done", 100);
    task->set_completed();

    con.print_widget(
        progress_builder().add_task(task).bar_width(10).complete_char('#').remaining_char('.'),
        80);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Done") != std::string::npos);
    EXPECT_TRUE(out.find("100%") != std::string::npos);
}

TEST(ProgressBuilderTest, MultipleTasks) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    auto t1 = std::make_shared<progress_task>("Task A", 100);
    auto t2 = std::make_shared<progress_task>("Task B", 200);
    t1->advance(25);
    t2->advance(100);

    con.print_widget(
        progress_builder().add_task(t1).add_task(t2).bar_width(10).complete_char('#').remaining_char('.'),
        80);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Task A") != std::string::npos);
    EXPECT_TRUE(out.find("Task B") != std::string::npos);
}

TEST(ProgressBuilderTest, EmptyTasks) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    con.print_widget(progress_builder(), 80);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("no tasks") != std::string::npos);
}
