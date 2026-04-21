/**
 * @file classic_app_demo.cpp
 * @brief Classic application layout using the classic_app library component.
 *
 * Demonstrates:
 *   - Horizontal menu bar with mouse-clickable labels
 *   - Pulldown multi-level submenus (keyboard + mouse)
 *   - Scrollable document content area with text selection
 *   - Status bar with live information
 *
 * Build:
 *   cmake --build build --target tapiru_classic_app_demo --config Release
 * Run:
 *   .\build\bin\Release\tapiru_classic_app_demo.exe
 */

#include "tapiru/widgets/classic_app.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
//  Menu definitions
// ═══════════════════════════════════════════════════════════════════════

static std::vector<menu_bar_entry> build_menus() {
    std::vector<menu_bar_entry> menus;

    // File
    menus.push_back({"File",
                     {
                         menu_item_builder("New File").shortcut("Ctrl+N"),
                         menu_item_builder("New Window").shortcut("Ctrl+Shift+N"),
                         menu_item_builder::separator(),
                         menu_item_builder("Open File...").shortcut("Ctrl+O"),
                         menu_item_builder("Open Folder...").shortcut("Ctrl+K Ctrl+O"),
                         menu_item_builder("Open Recent")
                             .add(menu_item_builder("project_alpha"))
                             .add(menu_item_builder("my_website"))
                             .add(menu_item_builder("tapiru"))
                             .add(menu_item_builder("game_engine")),
                         menu_item_builder::separator(),
                         menu_item_builder("Save").shortcut("Ctrl+S"),
                         menu_item_builder("Save As...").shortcut("Ctrl+Shift+S"),
                         menu_item_builder("Save All").shortcut("Ctrl+K S"),
                         menu_item_builder::separator(),
                         menu_item_builder("Exit").shortcut("Ctrl+Q"),
                     }});

    // Edit
    menus.push_back({"Edit",
                     {
                         menu_item_builder("Undo").shortcut("Ctrl+Z"),
                         menu_item_builder("Redo").shortcut("Ctrl+Y"),
                         menu_item_builder::separator(),
                         menu_item_builder("Cut").shortcut("Ctrl+X"),
                         menu_item_builder("Copy").shortcut("Ctrl+C"),
                         menu_item_builder("Paste").shortcut("Ctrl+V"),
                         menu_item_builder::separator(),
                         menu_item_builder("Find").shortcut("Ctrl+F"),
                         menu_item_builder("Replace").shortcut("Ctrl+H"),
                         menu_item_builder("Find in Files").shortcut("Ctrl+Shift+F"),
                     }});

    // View
    menus.push_back({"View",
                     {
                         menu_item_builder("Command Palette...").shortcut("Ctrl+Shift+P"),
                         menu_item_builder("Open View")
                             .add(menu_item_builder("Explorer"))
                             .add(menu_item_builder("Search"))
                             .add(menu_item_builder("Source Control"))
                             .add(menu_item_builder("Debug"))
                             .add(menu_item_builder("Extensions")),
                         menu_item_builder::separator(),
                         menu_item_builder("Appearance")
                             .add(menu_item_builder("Full Screen").shortcut("F11"))
                             .add(menu_item_builder("Zen Mode").shortcut("Ctrl+K Z"))
                             .add(menu_item_builder("Centered Layout")),
                         menu_item_builder("Editor Layout")
                             .add(menu_item_builder("Split Up"))
                             .add(menu_item_builder("Split Down"))
                             .add(menu_item_builder("Split Left"))
                             .add(menu_item_builder("Split Right")),
                         menu_item_builder::separator(),
                         menu_item_builder("Zoom In").shortcut("Ctrl++"),
                         menu_item_builder("Zoom Out").shortcut("Ctrl+-"),
                         menu_item_builder("Reset Zoom").shortcut("Ctrl+0"),
                     }});

    // Terminal
    menus.push_back({"Terminal",
                     {
                         menu_item_builder("New Terminal").shortcut("Ctrl+`"),
                         menu_item_builder("Split Terminal"),
                         menu_item_builder::separator(),
                         menu_item_builder("Run Task..."),
                         menu_item_builder("Run Build Task...").shortcut("Ctrl+Shift+B"),
                     }});

    // Help
    menus.push_back({"Help",
                     {
                         menu_item_builder("Welcome"),
                         menu_item_builder("Documentation"),
                         menu_item_builder("Release Notes"),
                         menu_item_builder::separator(),
                         menu_item_builder("Report Issue"),
                         menu_item_builder("About"),
                     }});

    return menus;
}

// ═══════════════════════════════════════════════════════════════════════
//  Document content (simulated)
// ═══════════════════════════════════════════════════════════════════════

static std::vector<std::string> generate_document() {
    std::vector<std::string> lines;
    lines.push_back("[bold bright_cyan]// tapiru \xe2\x80\x94 Classic App Demo[/]");
    lines.push_back("");
    lines.push_back("[bold]#include[/] [green]<tapiru/core/console.h>[/]");
    lines.push_back("[bold]#include[/] [green]<tapiru/widgets/menu.h>[/]");
    lines.push_back("[bold]#include[/] [green]<tapiru/widgets/status_bar.h>[/]");
    lines.push_back("");
    lines.push_back("[bold bright_magenta]using namespace[/] tapiru;");
    lines.push_back("");
    lines.push_back("[bold bright_magenta]int[/] main() {");
    lines.push_back("    console con;");
    lines.push_back("    con.print([green]\"Hello, tapiru!\"[/]);");
    lines.push_back("    [bold bright_magenta]return[/] [bright_yellow]0[/];");
    lines.push_back("}");
    lines.push_back("");
    lines.push_back("[bold bright_yellow]// \xe2\x94\x80\xe2\x94\x80 Hyperlink demos (rendered as overlay) "
                    "\xe2\x94\x80\xe2\x94\x80[/]");
    lines.push_back("");
    lines.push_back("[dim]OSC 8 hyperlink:[/]  (click me in Windows Terminal)");
    lines.push_back("[dim]OSC 8 mailto:[/]     (click to send email)");
    lines.push_back("[dim]OSC 8 file:[/]       (click to open local path)");
    lines.push_back("");
    lines.push_back("[bold bright_yellow]// \xe2\x94\x80\xe2\x94\x80 Sixel image (rendered as overlay) "
                    "\xe2\x94\x80\xe2\x94\x80[/]");
    lines.push_back("");
    // Reserve blank lines for the Sixel image area (10 rows)
    for (int i = 0; i < 10; ++i) {
        lines.push_back("");
    }
    lines.push_back("");
    lines.push_back("[dim]// End of demo content[/]");
    return lines;
}

// \xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90
//  Sixel image generator
// \xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90

// Generate a Sixel-encoded gradient image (width x height pixels).
// Produces a rainbow gradient from left to right, with vertical brightness.
static std::string generate_sixel_image(int width, int height) {
    // Sixel encodes 6 vertical pixels per row-band.
    // Each color is registered with #N;2;R;G;B (percent 0-100).
    // Pixel data: character = 63 + 6-bit mask of which of the 6 rows are set.

    std::string s;
    s.reserve(static_cast<size_t>(width * height * 2));

    // DCS q  — enter Sixel mode (P1=0 aspect ratio, P2=1 no background)
    s += "\x1bPq";

    // Register a palette of 64 colors (rainbow hues)
    for (int i = 0; i < 64; ++i) {
        double hue = static_cast<double>(i) / 64.0 * 360.0;
        // HSV to RGB (S=100%, V=100%)
        double c = 1.0;
        double x = 1.0 - std::fabs(std::fmod(hue / 60.0, 2.0) - 1.0);
        double r = 0, g = 0, b = 0;
        if (hue < 60) {
            r = c;
            g = x;
        } else if (hue < 120) {
            r = x;
            g = c;
        } else if (hue < 180) {
            g = c;
            b = x;
        } else if (hue < 240) {
            g = x;
            b = c;
        } else if (hue < 300) {
            r = x;
            b = c;
        } else {
            r = c;
            b = x;
        }
        char reg[32];
        std::snprintf(reg, sizeof(reg), "#%d;2;%d;%d;%d", i, static_cast<int>(r * 100), static_cast<int>(g * 100),
                      static_cast<int>(b * 100));
        s += reg;
    }

    // Emit pixel data in 6-row bands
    int bands = (height + 5) / 6;
    for (int band = 0; band < bands; ++band) {
        for (int ci = 0; ci < 64; ++ci) {
            // Select color
            char sel[8];
            std::snprintf(sel, sizeof(sel), "#%d", ci);
            s += sel;

            // For each column, determine which of the 6 rows in this band
            // should be painted with this color
            for (int col = 0; col < width; ++col) {
                int color_idx = col * 64 / width;
                uint8_t mask = 0;
                if (color_idx == ci) {
                    // Paint all 6 rows in this band that are within image height
                    for (int r = 0; r < 6; ++r) {
                        int py = band * 6 + r;
                        if (py < height) {
                            mask |= (1 << r);
                        }
                    }
                }
                s += static_cast<char>(63 + mask);
            }
            s += '$'; // carriage return (same band, next color)
        }
        s += '-'; // next band (line feed)
    }

    s += "\x1b\\"; // ST — exit Sixel mode
    return s;
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    auto doc = generate_document();

    classic_app app(classic_app::config{
        .menus = build_menus(),
        .theme = classic_app_theme::dark(),
    });

    app.set_content_lines(static_cast<int>(doc.size()));
    app.set_document_lines(doc);

    app.set_content([&](rows_builder &content, int scroll_y, int viewport_h) {
        int start = scroll_y;
        int end_line = start + viewport_h;
        if (end_line > static_cast<int>(doc.size())) {
            end_line = static_cast<int>(doc.size());
        }

        for (int i = start; i < end_line; ++i) {
            int screen_row = i - start + 1;
            bool is_cursor = (screen_row == app.cursor_line());
            char buf[256];
            if (is_cursor) {
                std::snprintf(buf, sizeof(buf), "[on_rgb(38,79,120)] [dim]%3d[/][on_rgb(38,79,120)] \xe2\x94\x82 %s[/]",
                              i + 1, doc[static_cast<size_t>(i)].c_str());
            } else {
                std::snprintf(buf, sizeof(buf), " [dim]%3d[/] \xe2\x94\x82 %s", i + 1,
                              doc[static_cast<size_t>(i)].c_str());
            }
            content.add(text_builder(buf));
        }

        for (int i = end_line - start; i < viewport_h; ++i) {
            content.add(text_builder(" [dim]  ~[/]"));
        }
    });

    app.set_status([&](status_bar_builder &sb) {
        char left_buf[128];
        std::snprintf(left_buf, sizeof(left_buf), " [bold]Ready[/]");

        char center_buf[128];
        if (app.has_selection()) {
            int sl, sc, el, ec;
            app.selection_range(sl, sc, el, ec);
            int sel_lines = el - sl + 1;
            if (sel_lines == 1) {
                std::snprintf(center_buf, sizeof(center_buf), "Ln %d, Col %d  (%d selected)",
                              app.cursor_line() + app.scroll_y(), app.cursor_col(), ec - sc);
            } else {
                std::snprintf(center_buf, sizeof(center_buf), "Ln %d, Col %d  (%d lines selected)",
                              app.cursor_line() + app.scroll_y(), app.cursor_col(), sel_lines);
            }
        } else {
            std::snprintf(center_buf, sizeof(center_buf), "Ln %d, Col %d", app.cursor_line() + app.scroll_y(),
                          app.cursor_col());
        }

        char right_buf[128];
        std::snprintf(right_buf, sizeof(right_buf), "UTF-8  C++  %d lines ", static_cast<int>(doc.size()));

        sb.left(left_buf);
        sb.center(center_buf);
        sb.right(right_buf);
    });

    app.on_menu_action([&](int /*menu_index*/, int /*item_global_index*/, const std::string &label) {
        app.set_status_message("Selected: " + label);
    });

    // Pre-generate the Sixel image (60 x 48 pixels ~ 8 terminal rows)
    std::string sixel_data = generate_sixel_image(60, 48);

    // OSC 8 String Terminator: ESC backslash
    static const char k_st[] = "\x1b\x5c"; // \x1b + '\'

    // Overlay: Sixel image + hyperlinks, rendered as raw ANSI after widget paint
    // Hyperlink doc lines start at line index 16 in the document.
    static constexpr int k_link_doc_line = 16;  // "OSC 8 hyperlink:" line
    static constexpr int k_sixel_doc_line = 22; // first blank line for Sixel
    static constexpr int k_gutter = 7;

    struct link_def {
        int line_offset; // relative to k_link_doc_line
        int col;         // column after gutter
        const char *url;
        const char *text;
        const char *ansi_style; // ANSI style prefix
    };
    static const link_def links[] = {
        {0, 21, "https://github.com/nicebyte/tapiru", "tapiru on GitHub",
         "\x1b[4;38;2;86;156;214m"},                                                 // underline + blue
        {1, 21, "mailto:dev@tapiru.io", "dev@tapiru.io", "\x1b[4;38;2;78;201;176m"}, // underline + teal
        {2, 21, "file:///C:/Windows/System32",
         "C:"
         "\x5c"
         "Windows"
         "\x5c"
         "System32",
         "\x1b[4;38;2;206;145;120m"}, // underline + orange
    };

    app.set_overlay([&](std::string &buf, uint32_t /*tw*/, uint32_t /*th*/, int scroll_y, int viewport_h) {
        // OSC 8 hyperlinks
        for (const auto &lk : links) {
            int doc_line = k_link_doc_line + lk.line_offset;
            int screen_line = doc_line - scroll_y;
            if (screen_line < 0 || screen_line >= viewport_h) {
                continue;
            }
            int screen_row = screen_line + 1; // +1 for menu bar

            char pos[24];
            std::snprintf(pos, sizeof(pos), "\x1b[%d;%dH", screen_row + 1, k_gutter + lk.col + 1);
            buf += pos;
            buf += lk.ansi_style;
            // OSC 8 open: ESC ] 8 ; ; URL ST
            buf += "\x1b]8;;";
            buf += lk.url;
            buf += k_st;
            buf += lk.text;
            // OSC 8 close: ESC ] 8 ; ; ST
            buf += "\x1b]8;;";
            buf += k_st;
            buf += "\x1b[0m";
        }

        // Sixel image
        int sixel_screen_line = k_sixel_doc_line - scroll_y;
        if (sixel_screen_line >= 0 && sixel_screen_line < viewport_h) {
            int screen_row = sixel_screen_line + 1; // +1 for menu bar
            char pos[24];
            std::snprintf(pos, sizeof(pos), "\x1b[%d;%dH", screen_row + 1, k_gutter + 1);
            buf += pos;
            buf += sixel_data;
        }
    });

    app.run();

    std::printf("Thanks for trying the classic app layout demo!\n");
    return 0;
}
