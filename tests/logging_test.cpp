#include "tapiru/core/console.h"
#include "tapiru/core/logging.h"

#include <gtest/gtest.h>
#include <mutex>
#include <string>

using namespace tapiru;

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

// ── log_level helpers ───────────────────────────────────────────────────

TEST(LoggingTest, LevelNames) {
    EXPECT_EQ(log_level_name(log_level::trace), "TRACE");
    EXPECT_EQ(log_level_name(log_level::debug), "DEBUG");
    EXPECT_EQ(log_level_name(log_level::info), "INFO");
    EXPECT_EQ(log_level_name(log_level::warn), "WARN");
    EXPECT_EQ(log_level_name(log_level::error), "ERROR");
    EXPECT_EQ(log_level_name(log_level::fatal), "FATAL");
}

TEST(LoggingTest, LevelStyles) {
    EXPECT_FALSE(log_level_style(log_level::trace).empty());
    EXPECT_FALSE(log_level_style(log_level::debug).empty());
    EXPECT_FALSE(log_level_style(log_level::info).empty());
    EXPECT_FALSE(log_level_style(log_level::warn).empty());
    EXPECT_FALSE(log_level_style(log_level::error).empty());
    EXPECT_FALSE(log_level_style(log_level::fatal).empty());
}

// ── log_handler ─────────────────────────────────────────────────────────

TEST(LoggingTest, BasicInfo) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.info("Server started");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("INFO") != std::string::npos);
    EXPECT_TRUE(out.find("Server started") != std::string::npos);
}

TEST(LoggingTest, ErrorLevel) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.error("Connection failed");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("ERROR") != std::string::npos);
    EXPECT_TRUE(out.find("Connection failed") != std::string::npos);
}

TEST(LoggingTest, LevelFiltering) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.set_level(log_level::warn);

    logger.debug("should not appear");
    logger.info("should not appear");
    logger.warn("this should appear");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("should not appear") == std::string::npos);
    EXPECT_TRUE(out.find("this should appear") != std::string::npos);
}

TEST(LoggingTest, AllLevels) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);

    logger.trace("t");
    logger.debug("d");
    logger.info("i");
    logger.warn("w");
    logger.error("e");
    logger.fatal("f");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("TRACE") != std::string::npos);
    EXPECT_TRUE(out.find("DEBUG") != std::string::npos);
    EXPECT_TRUE(out.find("INFO") != std::string::npos);
    EXPECT_TRUE(out.find("WARN") != std::string::npos);
    EXPECT_TRUE(out.find("ERROR") != std::string::npos);
    EXPECT_TRUE(out.find("FATAL") != std::string::npos);
}

TEST(LoggingTest, WithTimestamp) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(true);
    logger.info("timed");

    auto out = sink->snapshot();
    // Should contain a colon from HH:MM:SS
    EXPECT_TRUE(out.find(':') != std::string::npos);
    EXPECT_TRUE(out.find("timed") != std::string::npos);
}

TEST(LoggingTest, HideLevel) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.show_level(false);
    logger.info("bare message");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("INFO") == std::string::npos);
    EXPECT_TRUE(out.find("bare message") != std::string::npos);
}

TEST(LoggingTest, LogRecord) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);

    log_record rec;
    rec.level = log_level::warn;
    rec.message = "custom record";
    logger.log(rec);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("WARN") != std::string::npos);
    EXPECT_TRUE(out.find("custom record") != std::string::npos);
}

// ── Structured fields ──────────────────────────────────────────────────

TEST(LoggingTest, StructuredFields) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.log_structured(log_level::info, "Request handled", {{"status", "200"}, {"duration", "42ms"}});

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("Request handled") != std::string::npos);
    EXPECT_TRUE(out.find("status=200") != std::string::npos);
    EXPECT_TRUE(out.find("duration=42ms") != std::string::npos);
}

TEST(LoggingTest, RecordWithFields) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);

    log_record rec;
    rec.level = log_level::error;
    rec.message = "DB error";
    rec.fields = {{"table", "users"}};
    logger.log(rec);

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("DB error") != std::string::npos);
    EXPECT_TRUE(out.find("table=users") != std::string::npos);
}

// ── Module/tag filtering ───────────────────────────────────────────────

TEST(LoggingTest, ModuleFilter) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.allow_module("http");

    logger.set_module("http");
    logger.info("allowed");

    logger.set_module("db");
    logger.info("filtered out");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("allowed") != std::string::npos);
    EXPECT_TRUE(out.find("filtered out") == std::string::npos);
}

TEST(LoggingTest, TagFilter) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.allow_tag("perf");

    logger.set_tag("perf");
    logger.info("perf msg");

    logger.set_tag("security");
    logger.info("sec msg");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("perf msg") != std::string::npos);
    EXPECT_TRUE(out.find("sec msg") == std::string::npos);
}

TEST(LoggingTest, ClearFilters) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.allow_module("x");
    logger.set_module("y");
    logger.info("FILTERED_MSG");

    logger.clear_filters();
    logger.info("PASSED_MSG");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("FILTERED_MSG") == std::string::npos);
    EXPECT_TRUE(out.find("PASSED_MSG") != std::string::npos);
}

TEST(LoggingTest, ModulePrefixInOutput) {
    auto sink = std::make_shared<capture_sink>();
    console_config cfg;
    cfg.sink = [sink](std::string_view data) { (*sink)(data); };
    cfg.depth = color_depth::none;
    cfg.no_color = true;
    console con(cfg);

    log_handler logger(con);
    logger.show_timestamp(false);
    logger.set_module("http");
    logger.info("request");

    auto out = sink->snapshot();
    EXPECT_TRUE(out.find("http") != std::string::npos);
    EXPECT_TRUE(out.find("request") != std::string::npos);
}

// ── log_panel_builder ──────────────────────────────────────────────────

class virtual_terminal_log {
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

TEST(LogPanelTest, EmptyPanel) {
    virtual_terminal_log vt;
    auto con = vt.make_console(false, 80);
    log_panel_builder panel(10);
    con.print_widget(panel, 80);
    EXPECT_TRUE(vt.raw().find("no logs") != std::string::npos);
}

TEST(LogPanelTest, PushAndRender) {
    virtual_terminal_log vt;
    auto con = vt.make_console(false, 80);
    log_panel_builder panel(10);

    log_record rec;
    rec.level = log_level::info;
    rec.message = "Hello log";
    panel.push(rec);

    con.print_widget(panel, 80);
    EXPECT_TRUE(vt.raw().find("Hello log") != std::string::npos);
}

TEST(LogPanelTest, MaxLinesEviction) {
    log_panel_builder panel(3);
    for (int i = 0; i < 5; ++i) {
        log_record rec;
        rec.level = log_level::info;
        rec.message = "msg" + std::to_string(i);
        panel.push(rec);
    }
    EXPECT_EQ(panel.size(), 3u);
}

TEST(LogPanelTest, ClearPanel) {
    log_panel_builder panel(10);
    log_record rec;
    rec.level = log_level::info;
    rec.message = "x";
    panel.push(rec);
    EXPECT_EQ(panel.size(), 1u);
    panel.clear();
    EXPECT_EQ(panel.size(), 0u);
}

TEST(LogPanelTest, VisibleLinesLimit) {
    virtual_terminal_log vt;
    auto con = vt.make_console(false, 80);
    log_panel_builder panel(100);
    panel.visible_lines(2);

    for (int i = 0; i < 10; ++i) {
        log_record rec;
        rec.level = log_level::info;
        rec.message = "line" + std::to_string(i);
        panel.push(rec);
    }

    con.print_widget(panel, 80);
    auto &out = vt.raw();
    // Auto-scroll: should show last 2 lines
    EXPECT_TRUE(out.find("line9") != std::string::npos);
    EXPECT_TRUE(out.find("line8") != std::string::npos);
}
