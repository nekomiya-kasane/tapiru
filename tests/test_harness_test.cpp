#include <gtest/gtest.h>

#include "tapiru/testing/test_harness.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/tree_view.h"
#include "tapiru/widgets/tab.h"
#include "tapiru/widgets/breadcrumb.h"

using namespace tapiru;
using namespace tapiru::testing;

// ── virtual_screen basic tests ──────────────────────────────────────────

TEST(VirtualScreenTest, TextBuilder) {
    virtual_screen vs(40, 10);
    vs.render(text_builder("Hello World"));
    EXPECT_EQ(vs.row_count(), 1u);
    EXPECT_TRUE(vs.contains("Hello World"));
    EXPECT_EQ(vs.find_row("Hello"), 0);
}

TEST(VirtualScreenTest, RowsBuilder) {
    virtual_screen vs(40, 10);
    rows_builder rb;
    rb.gap(0);
    rb.add(text_builder("Line 1"));
    rb.add(text_builder("Line 2"));
    rb.add(text_builder("Line 3"));
    vs.render(rb);
    EXPECT_EQ(vs.row_count(), 3u);
    EXPECT_TRUE(vs.contains("Line 1"));
    EXPECT_TRUE(vs.contains("Line 2"));
    EXPECT_TRUE(vs.contains("Line 3"));
}

TEST(VirtualScreenTest, ColumnsBuilder) {
    virtual_screen vs(40, 10);
    columns_builder cb;
    cb.gap(1);
    cb.add(text_builder("Left"), 0);
    cb.add(text_builder("Right"), 1);
    vs.render(cb);
    EXPECT_EQ(vs.row_count(), 1u);
    EXPECT_TRUE(vs.contains("Left"));
    EXPECT_TRUE(vs.contains("Right"));
}

TEST(VirtualScreenTest, PanelBuilder) {
    virtual_screen vs(40, 10);
    panel_builder pb(text_builder("Content"));
    pb.title("My Panel");
    pb.border(border_style::rounded);
    vs.render(pb);
    // Title is rendered inside border line; content is inside the panel
    EXPECT_TRUE(vs.contains("Content"));
    // Panel should have at least 3 rows (top border, content, bottom border)
    EXPECT_GE(vs.row_count(), 3u);
}

TEST(VirtualScreenTest, SizedBoxHeight) {
    virtual_screen vs(40, 10);
    sized_box_builder sb(text_builder("X"));
    sb.height(5);
    vs.render(sb);
    EXPECT_EQ(vs.row_count(), 5u);
}

TEST(VirtualScreenTest, TreeView) {
    virtual_screen vs(40, 10);
    tree_node root;
    root.label = "Root";
    tree_node child;
    child.label = "Child";
    root.children.push_back(std::move(child));

    std::unordered_set<std::string> expanded = {"Root"};
    int cursor = 0;

    auto tree = tree_view_builder()
        .root(std::move(root))
        .expanded_set(&expanded)
        .cursor(&cursor);
    vs.render(tree);
    EXPECT_TRUE(vs.contains("Root"));
    EXPECT_TRUE(vs.contains("Child"));
}

// ── app_harness tests ───────────────────────────────────────────────────

TEST(AppHarnessTest, MenuBarVisible) {
    app_harness ah(app_harness::config{
        .width = 80,
        .height = 24,
        .menus = {
            {"File", {}},
            {"View", {}},
            {"Help", {}},
        },
    });
    ah.set_content([](rows_builder& content, int, int vh) {
        sized_box_builder sb(text_builder("Content"));
        sb.height(static_cast<uint32_t>(vh));
        content.add(std::move(sb));
    });
    ah.set_status([](status_bar_builder& sb) {
        sb.left("Status Left");
        sb.right("Status Right");
    });
    ah.render();

    auto& scr = ah.screen();
    // Menu bar should be on row 0
    EXPECT_TRUE(scr.contains("File"));
    EXPECT_TRUE(scr.contains("View"));
    EXPECT_TRUE(scr.contains("Help"));
    EXPECT_EQ(scr.find_row("File"), 0);

    // Status bar should be on the last row
    EXPECT_TRUE(scr.contains("Status Left"));
    EXPECT_TRUE(scr.contains("Status Right"));

    // Content should be present
    EXPECT_TRUE(scr.contains("Content"));
}

TEST(AppHarnessTest, FrameHeight) {
    app_harness ah(app_harness::config{
        .width = 80,
        .height = 24,
    });
    ah.set_content([](rows_builder& content, int, int vh) {
        sized_box_builder sb(text_builder("X"));
        sb.height(static_cast<uint32_t>(vh));
        content.add(std::move(sb));
    });
    ah.set_status([](status_bar_builder& sb) {
        sb.left("S");
    });
    ah.render();

    auto& scr = ah.screen();
    // menu(1) + viewport_h(21) + status(1) = 23
    if (scr.row_count() != 23u) {
        scr.dump(std::cerr);
    }
    EXPECT_EQ(scr.row_count(), 23u);
}

TEST(AppHarnessTest, FrameHeightWithTable) {
    // Simulate ss_viewer layout: tree panel + table in columns, wrapped in sized_box
    app_harness ah(app_harness::config{
        .width = 80,
        .height = 24,
        .menus = {{"File", {}}, {"View", {}}, {"Help", {}}},
    });
    ah.set_content([](rows_builder& content, int, int vh) {
        // Left: panel with some text
        panel_builder tree_panel(text_builder("Root\n  Child1\n  Child2"));
        tree_panel.title("Tree");
        tree_panel.border(border_style::rounded);

        // Right: table
        table_builder tb;
        tb.add_column("Property", {justify::left, 16, 20});
        tb.add_column("Value",    {justify::left, 20, 40});
        tb.border(border_style::rounded);
        tb.add_row({"Name", "Root Entry"});
        tb.add_row({"Type", "Root Storage"});
        tb.add_row({"Size", "512 B"});

        rows_builder right_pane;
        right_pane.gap(0);
        right_pane.add(std::move(tb), 1);

        sized_box_builder tree_box(std::move(tree_panel));
        tree_box.min_width(32);

        columns_builder layout;
        layout.gap(1);
        layout.add(std::move(tree_box), 0);
        layout.add(std::move(right_pane), 1);

        sized_box_builder sized(std::move(layout));
        sized.height(static_cast<uint32_t>(std::max(vh, 1)));
        content.add(std::move(sized));
    });
    ah.set_status([](status_bar_builder& sb) {
        sb.left("file.cfb");
        sb.center("CFB v4");
        sb.right("Root Storage");
    });
    ah.render();

    auto& scr = ah.screen();
    // menu(1) + viewport_h(21) + status(1) = 23
    if (scr.row_count() != 23u) {
        scr.dump(std::cerr);
    }
    EXPECT_EQ(scr.row_count(), 23u);
    // Menu bar on row 0
    EXPECT_EQ(scr.find_row("File"), 0);
    // Status bar on last row
    EXPECT_TRUE(scr.contains("file.cfb"));
    EXPECT_TRUE(scr.contains("CFB v4"));
}
