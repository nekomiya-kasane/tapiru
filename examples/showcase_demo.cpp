/**
 * @file showcase_demo.cpp
 * @brief Non-interactive showcase of every tapiru feature.
 *
 * This demo prints a series of sections to stdout, each demonstrating
 * a different part of the tapiru library. No keyboard input required —
 * just run it and scroll through the output.
 *
 * Features demonstrated:
 *   1.  Terminal detection    — size, color depth, TTY, OSC support
 *   2.  Style system          — color, attr, style structs
 *   3.  ANSI emitter          — minimal-delta SGR transitions
 *   4.  Inline markup         — [bold red], [#RRGGBB], rgb(), color256(), semantic aliases
 *   5.  Constexpr markup      — ct_parse_markup, markup_style, static_assert
 *   6.  Rich markup (runtime) — print_rich with block-level tags
 *   7.  Rich markup (compile) — TAPIRU_PRINT_RICH macro
 *   8.  Widget: text_builder   — alignment, overflow, style override
 *   9.  Widget: rule_builder   — plain, titled, styled, gradient
 *  10.  Widget: panel_builder  — border styles, titles, shadows, gradients
 *  11.  Widget: table_builder  — columns, alignment, header style, border gradient
 *  12.  Widget: columns_builder — multi-column layout with flex
 *  13.  Widget: rows_builder    — vertical stacking with gap
 *  14.  Widget: padding_builder — padding around content
 *  15.  Widget: center_builder  — horizontal/vertical centering
 *  16.  Widget: sized_box_builder — fixed/min/max constraints
 *  17.  Widget: overlay_builder — layered compositing
 *  18.  Gradients              — linear_gradient on rules, panels, tables
 *  19.  Shaders                — scanline, shimmer, vignette, glow_pulse
 *  20.  Color downgrade        — RGB → 256 → 16 → none
 *  21.  OSC sequences          — hyperlinks, title, clipboard, shell integration
 *  22.  Low-level ANSI         — cursor movement, clear, SGR builder
 *
 * Build:
 *   cmake --build build --target tapiru_showcase_demo --config Release
 * Run:
 *   .\build\bin\Release\tapiru_showcase_demo.exe
 */

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#    define NOMINMAX
#    include <crtdbg.h>
#    include <windows.h>
#endif

#include "tapiru/core/ansi.h"
#include "tapiru/core/console.h"
#include "tapiru/core/gradient.h"
#include "tapiru/core/shader.h"
#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"
#include "tapiru/text/constexpr_markup.h"
#include "tapiru/widgets/builders.h"

using namespace tapiru;

// ════════════════════════════════════════════════════════════════════════
//  Helpers
// ════════════════════════════════════════════════════════════════════════

static void section_header(console &con, const char *number, const char *title) {
    con.newline();
    con.print_widget(rule_builder(std::string("[bold bright_cyan]") + number + ". " + title + "[/]")
                         .rule_style(style{colors::bright_black}));
    con.newline();
}

// ════════════════════════════════════════════════════════════════════════
//  1. Terminal Detection
// ════════════════════════════════════════════════════════════════════════

static void demo_terminal_detection(console &con) {
    section_header(con, "1", "Terminal Detection");

    auto sz = terminal::get_size();
    auto depth = terminal::detect_color_depth();
    bool tty = terminal::is_tty();
    auto osc = terminal::detect_osc_support();

    const char *depth_names[] = {"none", "16-color", "256-color", "true-color"};

    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "  Size: [bold cyan]%u[/] x [bold cyan]%u[/]    "
                  "Color depth: [bold green]%s[/]    "
                  "TTY: %s",
                  sz.width, sz.height, depth_names[static_cast<int>(depth)], tty ? "[bold green]yes[/]" : "[dim]no[/]");
    con.print(buf);

    std::string osc_line = "  OSC support: ";
    auto flag = [&](const char *name, bool v) {
        osc_line += v ? (std::string("[green]") + name + "[/] ") : (std::string("[dim]") + name + "[/] ");
    };
    flag("title", osc.title);
    flag("hyperlink", osc.hyperlink);
    flag("clipboard", osc.clipboard);
    flag("palette", osc.palette);
    flag("cwd", osc.cwd);
    flag("notify", osc.notify);
    flag("shell-int", osc.shell_integration);
    con.print(osc_line);
}

// ════════════════════════════════════════════════════════════════════════
//  2. Style System
// ════════════════════════════════════════════════════════════════════════

static void demo_style_system(console &con) {
    section_header(con, "2", "Style System (color + attr + style)");

    // Named 16-color palette
    con.print("  [bold]16-color ANSI palette:[/]");
    {
        std::string line = "  ";
        const char *names[] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};
        for (int i = 0; i < 8; ++i) {
            line += std::string("[on_") + names[i] + "]  [/]";
        }
        line += " ";
        const char *bright_names[] = {"bright_black", "bright_red",     "bright_green", "bright_yellow",
                                      "bright_blue",  "bright_magenta", "bright_cyan",  "bright_white"};
        for (int i = 0; i < 8; ++i) {
            line += std::string("[on_") + bright_names[i] + "]  [/]";
        }
        con.print(line);
    }

    // 256-color palette (sample)
    con.print("  [bold]256-color palette (sample):[/]");
    {
        std::string line = "  ";
        for (int i = 16; i < 52; ++i) {
            char tag[32];
            std::snprintf(tag, sizeof(tag), "[on_color256(%d)]  [/]", i);
            line += tag;
        }
        con.print(line);
        line = "  ";
        for (int i = 52; i < 88; ++i) {
            char tag[32];
            std::snprintf(tag, sizeof(tag), "[on_color256(%d)]  [/]", i);
            line += tag;
        }
        con.print(line);
    }

    // RGB true-color gradient
    con.print("  [bold]True-color RGB gradient:[/]");
    {
        std::string line = "  ";
        for (int i = 0; i < 60; ++i) {
            uint8_t r = static_cast<uint8_t>(i * 4);
            uint8_t g = static_cast<uint8_t>(60 + i * 2);
            uint8_t b = static_cast<uint8_t>(255 - i * 4);
            char tag[48];
            std::snprintf(tag, sizeof(tag), "[on_rgb(%u,%u,%u)] [/]", r, g, b);
            line += tag;
        }
        con.print(line);
    }

    // Text attributes
    con.print("  [bold]Text attributes:[/]");
    con.print("    [bold]bold[/]  [dim]dim[/]  [italic]italic[/]  [underline]underline[/]  "
              "[strike]strike[/]  [reverse]reverse[/]  [blink]blink[/]  "
              "[double_underline]double_underline[/]  [overline]overline[/]");
}

// ════════════════════════════════════════════════════════════════════════
//  3. ANSI Emitter (minimal-delta transitions)
// ════════════════════════════════════════════════════════════════════════

static void demo_ansi_emitter(console &con) {
    section_header(con, "3", "ANSI Emitter (minimal-delta SGR)");

    con.print("  The [bold]ansi_emitter[/] tracks hardware state and computes the");
    con.print("  [italic]shortest SGR transition[/] between styles (reset-vs-delta heuristic).");
    con.newline();

    // Demonstrate by manually building styled output
    ansi_emitter em;
    std::string out = "  ";

    style s1{colors::red, {}, attr::bold};
    style s2{colors::green, {}, attr::bold | attr::italic};
    style s3{colors::blue, {}, attr::underline};

    em.transition(s1, out);
    out += "Red Bold";
    em.transition(s2, out);
    out += " -> Green Bold Italic";
    em.transition(s3, out);
    out += " -> Blue Underline";
    em.reset(out);
    out += " -> Reset";

    con.write(out);
    con.newline();
    con.print("  [dim](Each transition emits only the changed SGR parameters)[/]");
}

// ════════════════════════════════════════════════════════════════════════
//  4. Inline Markup
// ════════════════════════════════════════════════════════════════════════

static void demo_inline_markup(console &con) {
    section_header(con, "4", "Inline Markup Tags");

    con.print("  [bold]Basic colors:[/]  [red]red[/] [green]green[/] [blue]blue[/] "
              "[yellow]yellow[/] [magenta]magenta[/] [cyan]cyan[/]");
    con.print("  [bold]Bright:[/]  [bright_red]bright_red[/] [bright_green]bright_green[/] "
              "[bright_blue]bright_blue[/] [bright_cyan]bright_cyan[/]");
    con.print("  [bold]Hex RGB:[/]  [#FF6600]#FF6600[/] [#00CCFF]#00CCFF[/] "
              "[#FF00FF on_#1E1E2E]#FF00FF on #1E1E2E[/]");
    con.print("  [bold]rgb() func:[/]  [rgb(255,100,50)]rgb(255,100,50)[/] "
              "[on_rgb(30,30,60) bright_white] on_rgb(30,30,60) [/]");
    con.print("  [bold]color256():[/]  [color256(196)]color256(196)[/] "
              "[color256(46)]color256(46)[/] [color256(21)]color256(21)[/]");
    con.print("  [bold]Compound:[/]  [bold italic underline bright_cyan]bold italic underline bright_cyan[/]");
    con.print("  [bold]Negation:[/]  [bold italic]bold italic[not_italic] -> not_italic[/]");
    con.newline();

    // Semantic aliases
    con.print("  [bold]Semantic aliases:[/]");
    con.print("    [error]error[/]  [warning]warning[/]  [success]success[/]  [info]info[/]  "
              "[debug]debug[/]  [muted]muted[/]  [highlight]highlight[/]");
    con.print("    [code_inline]code_inline[/]  [url]url[/]  [path]path[/]  [key] key [/]");
    con.print("    [badge.red] FAIL [/] [badge.green] PASS [/] [badge.blue] INFO [/] [badge.yellow] WARN [/]");
}

// ════════════════════════════════════════════════════════════════════════
//  5. Constexpr Markup
// ════════════════════════════════════════════════════════════════════════

static void demo_constexpr_markup(console &con) {
    section_header(con, "5", "Constexpr Markup (compile-time parsing)");

    // markup_style: resolve tag string to style at compile time
    constexpr auto s = markup_style("bold red on_blue");
    static_assert(s.fg == colors::red);
    static_assert(s.bg == colors::blue);
    static_assert(has_attr(s.attrs, attr::bold));

    con.print("  [bold]markup_style(\"bold red on_blue\")[/] resolved at compile time:");
    con.print("    fg == colors::red     [green]\xe2\x9c\x93[/]");
    con.print("    bg == colors::blue    [green]\xe2\x9c\x93[/]");
    con.print("    attrs has bold        [green]\xe2\x9c\x93[/]");

    // ct_parse_markup: full markup parsing at compile time
    constexpr auto m = ct_parse_markup("[bold]Hello[/] World");
    static_assert(m.count == 2);
    static_assert(has_attr(m.fragments[0].sty.attrs, attr::bold));
    static_assert(m.fragments[1].sty.attrs == attr::none);

    con.newline();
    con.print("  [bold]ct_parse_markup(\"[bold]Hello[/] World\")[/]:");
    con.print("    fragment count == 2   [green]\xe2\x9c\x93[/]");
    con.print("    frag[0] has bold      [green]\xe2\x9c\x93[/]");
    con.print("    frag[1] is unstyled   [green]\xe2\x9c\x93[/]");

    // compile_markup: rich block-level parsing at compile time
    constexpr auto r = compile_markup("[box]Content[/box]");
    static_assert(r.count >= 2); // box_open + text + box_close

    con.newline();
    con.print("  [bold]compile_markup(\"[box]Content[/box]\")[/]:");
    char buf[64];
    std::snprintf(buf, sizeof(buf), "    fragment count == %zu  [green]\xe2\x9c\x93[/]", r.count);
    con.print(buf);
    con.print("  [dim](All parsing happens at compile time — zero runtime overhead)[/]");
}

// ════════════════════════════════════════════════════════════════════════
//  6. Rich Markup (runtime)
// ════════════════════════════════════════════════════════════════════════

static void demo_rich_markup_runtime(console &con) {
    section_header(con, "6", "Rich Markup \xe2\x80\x94 Runtime (print_rich)");

    con.print("  [bold]Boxes:[/]");
    con.print_rich("[indent=4][box]Default rounded box[/box][/indent]");
    con.print_rich("[indent=4][box.heavy]Heavy border box[/box.heavy][/indent]");
    con.print_rich("[indent=4][box.double]Double border box[/box.double][/indent]");
    con.print_rich("[indent=4][box.ascii]ASCII border box[/box.ascii][/indent]");
    con.print_rich("[indent=4][box title=\"My Title\"]Box with a title[/box][/indent]");

    con.newline();
    con.print("  [bold]Rules:[/]");
    con.print_rich("[indent=4][rule][/indent]");
    con.print_rich("[indent=4][rule=Section Title][/indent]");

    con.newline();
    con.print("  [bold]Headings:[/]");
    con.print_rich("[indent=4][h1]Heading 1 (bold underline)[/h1]");
    con.print_rich("[indent=4][h2]Heading 2 (bold)[/h2]");
    con.print_rich("[indent=4][h3]Heading 3 (italic)[/h3][/indent]");

    con.newline();
    con.print("  [bold]Blockquote:[/]");
    con.print_rich("[indent=4][quote]This is a blockquote with a vertical bar prefix.[/quote][/indent]");

    con.newline();
    con.print("  [bold]Code block:[/]");
    con.print_rich("[indent=4][code]int main() { return 0; }[/code][/indent]");

    con.newline();
    con.print("  [bold]Progress bar:[/]");
    con.print_rich("[indent=4][progress=75][/indent]");

    con.newline();
    con.print("  [bold]Fraction bar:[/]");
    con.print_rich("[indent=4][bar=7/10][/indent]");

    con.newline();
    con.print("  [bold]Spacing & line breaks:[/]");
    con.print_rich("[indent=4]Before[sp=10]After 10 spaces[br]New line via [code_inline][br][/][/indent]");
}

// ════════════════════════════════════════════════════════════════════════
//  7. Rich Markup (compile-time via TAPIRU_PRINT_RICH)
// ════════════════════════════════════════════════════════════════════════

static void demo_rich_markup_constexpr(console &con) {
    section_header(con, "7", "Rich Markup \xe2\x80\x94 Compile-time (TAPIRU_PRINT_RICH)");

    con.print("  [dim]The TAPIRU_PRINT_RICH macro compiles markup at compile time[/]");
    con.print("  [dim]and executes pre-built render ops at runtime.[/]");
    con.newline();

    TAPIRU_PRINT_RICH(con, "    [bold bright_cyan]Styled text[/] from a [italic]compile-time[/] plan");
    TAPIRU_PRINT_RICH(con, "    [box title=\"Compile-Time Box\"][bold green]Zero-cost[/] markup parsing![/box]");
    TAPIRU_PRINT_RICH(con, "    [rule=Constexpr Rule]");
    TAPIRU_PRINT_RICH(con, "    [quote][italic]Compile-time quote block[/italic][/quote]");
    TAPIRU_PRINT_RICH(con, "    [progress=42]");
}

// ════════════════════════════════════════════════════════════════════════
//  8. Widget: text_builder
// ════════════════════════════════════════════════════════════════════════

static void demo_text_builder(console &con) {
    section_header(con, "8", "Widget: text_builder");

    {
        rows_builder rb;
        rb.add(text_builder("[bold]Left-aligned (default):[/] The quick brown fox"));
        rb.add(text_builder("[bold]Center-aligned:[/] The quick brown fox").align(justify::center));
        rb.add(text_builder("[bold]Right-aligned:[/] The quick brown fox").align(justify::right));
        rb.add(text_builder("[bold]Style override:[/] This text has a style override")
                   .style_override(style{colors::bright_yellow, {}, attr::italic}));
        rb.gap(0);
        con.print_widget(padding_builder(std::move(rb)).pad(0, 2));
    }
}

// ════════════════════════════════════════════════════════════════════════
//  9. Widget: rule_builder
// ════════════════════════════════════════════════════════════════════════

static void demo_rule_builder(console &con) {
    section_header(con, "9", "Widget: rule_builder");

    con.print("  [bold]Plain rule:[/]");
    con.print_widget(padding_builder(rule_builder()).pad(0, 2));

    con.print("  [bold]Titled rule (centered):[/]");
    con.print_widget(padding_builder(rule_builder("Section Title")).pad(0, 2));

    con.print("  [bold]Styled rule:[/]");
    con.print_widget(
        padding_builder(rule_builder("Styled").rule_style(style{colors::bright_magenta, {}, attr::bold})).pad(0, 2));

    con.print("  [bold]Gradient rule:[/]");
    con.print_widget(
        padding_builder(rule_builder("Gradient")
                            .gradient(linear_gradient{color::from_rgb(255, 0, 0), color::from_rgb(0, 0, 255),
                                                      gradient_direction::horizontal}))
            .pad(0, 2));
}

// ════════════════════════════════════════════════════════════════════════
// 10. Widget: panel_builder
// ════════════════════════════════════════════════════════════════════════

static void demo_panel_builder(console &con) {
    section_header(con, "10", "Widget: panel_builder");

    // Border styles
    con.print("  [bold]Border styles:[/]");
    {
        columns_builder cols;
        {
            panel_builder p(text_builder("[bold]Rounded[/]"));
            p.border(border_style::rounded);
            p.title("rounded");
            cols.add(std::move(p), 1);
        }
        {
            panel_builder p(text_builder("[bold]Heavy[/]"));
            p.border(border_style::heavy);
            p.title("heavy");
            cols.add(std::move(p), 1);
        }
        {
            panel_builder p(text_builder("[bold]Double[/]"));
            p.border(border_style::double_);
            p.title("double");
            cols.add(std::move(p), 1);
        }
        {
            panel_builder p(text_builder("[bold]ASCII[/]"));
            p.border(border_style::ascii);
            p.title("ascii");
            cols.add(std::move(p), 1);
        }
        cols.gap(1);
        con.print_widget(padding_builder(std::move(cols)).pad(0, 2));
    }

    // Panel with title and styled border
    con.print("  [bold]Styled panel with title:[/]");
    {
        rows_builder rb;
        rb.add(text_builder("[bold bright_cyan]tapiru[/] — Modern C++23 Terminal UI"));
        rb.add(text_builder("[dim]Rich widgets, markup, gradients, shaders[/]"));
        rb.gap(0);
        panel_builder pb(std::move(rb));
        pb.title("About");
        pb.border(border_style::rounded);
        pb.border_style_override(style{colors::bright_cyan});
        pb.title_style(style{colors::bright_white, {}, attr::bold});
        con.print_widget(padding_builder(std::move(pb)).pad(0, 2));
    }

    // Panel with shadow
    con.print("  [bold]Panel with shadow:[/]");
    {
        panel_builder pb(text_builder("[bold]Shadow effect[/] adds depth"));
        pb.title("Shadow");
        pb.shadow();
        con.print_widget(padding_builder(std::move(pb)).pad(0, 2));
    }

    // Panel with background gradient
    con.print("  [bold]Panel with background gradient:[/]");
    {
        panel_builder pb(text_builder("[bold bright_white]Gradient background[/]"));
        pb.title("Gradient");
        pb.background_gradient(
            linear_gradient{color::from_rgb(30, 30, 80), color::from_rgb(80, 30, 30), gradient_direction::horizontal});
        con.print_widget(padding_builder(std::move(pb)).pad(0, 2));
    }
}

// ════════════════════════════════════════════════════════════════════════
// 11. Widget: table_builder
// ════════════════════════════════════════════════════════════════════════

static void demo_table_builder(console &con) {
    section_header(con, "11", "Widget: table_builder");

    // Basic table
    con.print("  [bold]Basic table:[/]");
    {
        table_builder tb;
        tb.add_column("Feature", {justify::left, 20, 30});
        tb.add_column("Status", {justify::center, 10, 14});
        tb.add_column("Notes", {justify::left, 20, 40});
        tb.header_style(style{colors::bright_cyan, {}, attr::bold});
        tb.border(border_style::rounded);

        tb.add_row({"Style system", "[green]Done[/]", "color + attr + style"});
        tb.add_row({"ANSI emitter", "[green]Done[/]", "Minimal-delta SGR"});
        tb.add_row({"Markup parser", "[green]Done[/]", "Runtime + constexpr"});
        tb.add_row({"Widget builders", "[green]Done[/]", "12 widget types"});
        tb.add_row({"Gradients", "[green]Done[/]", "Linear H/V"});
        tb.add_row({"Shaders", "[green]Done[/]", "4 built-in effects"});
        tb.add_row({"Color downgrade", "[green]Done[/]", "RGB->256->16->none"});
        tb.add_row({"OSC sequences", "[green]Done[/]", "Hyperlinks, title, etc."});

        con.print_widget(padding_builder(std::move(tb)).pad(0, 2));
    }

    // Table with border gradient
    con.print("  [bold]Table with border gradient:[/]");
    {
        table_builder tb;
        tb.add_column("Color", {justify::left, 12, 16});
        tb.add_column("Hex", {justify::center, 10, 14});
        tb.add_column("RGB", {justify::center, 16, 20});
        tb.header_style(style{colors::bright_white, {}, attr::bold});
        tb.border(border_style::heavy);
        tb.border_gradient(
            linear_gradient{color::from_rgb(255, 100, 0), color::from_rgb(0, 100, 255), gradient_direction::vertical});

        tb.add_row({"[red]Red[/]", "#FF0000", "(255, 0, 0)"});
        tb.add_row({"[green]Green[/]", "#00FF00", "(0, 255, 0)"});
        tb.add_row({"[blue]Blue[/]", "#0000FF", "(0, 0, 255)"});
        tb.add_row({"[yellow]Yellow[/]", "#FFFF00", "(255, 255, 0)"});
        tb.add_row({"[magenta]Magenta[/]", "#FF00FF", "(255, 0, 255)"});

        con.print_widget(padding_builder(std::move(tb)).pad(0, 2));
    }
}

// ════════════════════════════════════════════════════════════════════════
// 12. Widget: columns_builder + rows_builder
// ════════════════════════════════════════════════════════════════════════

static void demo_layout_builders(console &con) {
    section_header(con, "12", "Layout: columns_builder + rows_builder");

    con.print("  [bold]Three-column layout with flex ratios:[/]");
    {
        columns_builder cols;
        {
            rows_builder r1;
            r1.add(text_builder("[bold bright_red]Column 1[/]"));
            r1.add(text_builder("flex = 1"));
            r1.gap(0);
            auto p1 = panel_builder(std::move(r1));
            p1.title("Left");
            p1.border(border_style::rounded);
            cols.add(std::move(p1), 1);
        }
        {
            rows_builder r2;
            r2.add(text_builder("[bold bright_green]Column 2[/]"));
            r2.add(text_builder("flex = 2 (wider)"));
            r2.gap(0);
            auto p2 = panel_builder(std::move(r2));
            p2.title("Center");
            p2.border(border_style::rounded);
            cols.add(std::move(p2), 2);
        }
        {
            rows_builder r3;
            r3.add(text_builder("[bold bright_blue]Column 3[/]"));
            r3.add(text_builder("flex = 1"));
            r3.gap(0);
            auto p3 = panel_builder(std::move(r3));
            p3.title("Right");
            p3.border(border_style::rounded);
            cols.add(std::move(p3), 1);
        }
        cols.gap(1);
        con.print_widget(padding_builder(std::move(cols)).pad(0, 2));
    }

    con.print("  [bold]Nested rows with gap:[/]");
    {
        rows_builder rows;
        rows.add(text_builder("  [bold bright_cyan]Row 1[/] — gap(1) between rows"));
        rows.add(text_builder("  [bold bright_green]Row 2[/] — automatic vertical stacking"));
        rows.add(text_builder("  [bold bright_yellow]Row 3[/] — with consistent spacing"));
        rows.gap(1);
        con.print_widget(std::move(rows));
    }
}

// ════════════════════════════════════════════════════════════════════════
// 13. Widget: padding, center, sized_box, overlay
// ════════════════════════════════════════════════════════════════════════

static void demo_utility_builders(console &con) {
    section_header(con, "13", "Utility Widgets: padding, center, sized_box, overlay");

    con.print("  [bold]padding_builder — pad(1, 3):[/]");
    {
        panel_builder pb(text_builder("Padded content"));
        pb.border(border_style::rounded);
        con.print_widget(padding_builder(std::move(pb)).pad(1, 3));
    }

    con.print("  [bold]center_builder — horizontal centering:[/]");
    {
        panel_builder pb(text_builder("[bold]Centered panel[/]"));
        pb.border(border_style::rounded);
        center_builder cb(std::move(pb));
        cb.horizontal_only();
        con.print_widget(std::move(cb));
    }

    con.print("  [bold]sized_box_builder — fixed width 40:[/]");
    {
        panel_builder pb(text_builder("Width constrained to 40"));
        pb.border(border_style::rounded);
        sized_box_builder sb(std::move(pb));
        sb.width(40);
        padding_builder pad(std::move(sb));
        pad.pad(0, 2);
        con.print_widget(std::move(pad));
    }

    con.print("  [bold]overlay_builder — layered compositing:[/]");
    {
        panel_builder base_pb(text_builder("[dim]Background layer (base)[/]"));
        base_pb.border(border_style::rounded);
        overlay_builder ob(std::move(base_pb), text_builder("[bold bright_white on_red] OVERLAY [/]"));
        con.print_widget(padding_builder(std::move(ob)).pad(0, 2));
    }
}

// ════════════════════════════════════════════════════════════════════════
// 14. Gradients
// ════════════════════════════════════════════════════════════════════════

static void demo_gradients(console &con) {
    section_header(con, "14", "Linear Gradients");

    con.print("  [bold]Horizontal gradient (red -> blue):[/]");
    con.print_widget(
        padding_builder(rule_builder("Horizontal")
                            .gradient(linear_gradient{color::from_rgb(255, 0, 0), color::from_rgb(0, 0, 255),
                                                      gradient_direction::horizontal}))
            .pad(0, 2));

    con.print("  [bold]Panel with vertical gradient background:[/]");
    {
        rows_builder rb;
        rb.add(text_builder("[bright_white]Line 1[/]"));
        rb.add(text_builder("[bright_white]Line 2[/]"));
        rb.add(text_builder("[bright_white]Line 3[/]"));
        rb.gap(0);
        panel_builder pb(std::move(rb));
        pb.title("Vertical Gradient");
        pb.background_gradient(
            linear_gradient{color::from_rgb(0, 40, 80), color::from_rgb(80, 0, 40), gradient_direction::vertical});
        con.print_widget(padding_builder(std::move(pb)).pad(0, 2));
    }

    con.print("  [bold]Table with gradient border:[/]");
    {
        table_builder tb;
        tb.add_column("Gradient", {justify::center, 20, 30});
        tb.add_column("Direction", {justify::center, 15, 20});
        tb.border(border_style::heavy);
        tb.header_style(style{colors::bright_white, {}, attr::bold});
        tb.border_gradient(
            linear_gradient{color::from_rgb(0, 255, 128), color::from_rgb(128, 0, 255), gradient_direction::vertical});
        tb.add_row({"Green -> Purple", "Vertical"});
        con.print_widget(padding_builder(std::move(tb)).pad(0, 2));
    }
}

// ════════════════════════════════════════════════════════════════════════
// 15. Shaders
// ════════════════════════════════════════════════════════════════════════

static void demo_shaders(console &con) {
    section_header(con, "15", "Shaders (post-processing effects)");

    con.print("  [bold]Built-in shaders:[/]");
    con.print("    [bold cyan]shaders::scanline(0.3)[/]    — CRT scanline effect");
    con.print("    [bold cyan]shaders::shimmer(c, 2.0)[/]  — Animated diagonal shimmer");
    con.print("    [bold cyan]shaders::vignette(0.5)[/]    — Edge darkening");
    con.print("    [bold cyan]shaders::glow_pulse(c, 60)[/] — Pulsing border glow");
    con.newline();

    con.print("  [bold]Panel with scanline shader:[/]");
    {
        rows_builder rb;
        rb.add(text_builder("[bold bright_green]CRT Scanline Effect[/]"));
        rb.add(text_builder("Every other row is dimmed"));
        rb.add(text_builder("Simulates old CRT monitors"));
        rb.gap(0);
        panel_builder pb(std::move(rb));
        pb.title("Scanline");
        pb.border(border_style::rounded);
        pb.shader(shaders::scanline(0.3f));
        con.print_widget(padding_builder(std::move(pb)).pad(0, 2));
    }

    con.print("  [bold]Panel with vignette shader:[/]");
    {
        rows_builder rb;
        rb.add(text_builder("[bold bright_cyan]Vignette Effect[/]"));
        rb.add(text_builder("Edges are darkened"));
        rb.add(text_builder("Creates a spotlight feel"));
        rb.gap(0);
        panel_builder pb(std::move(rb));
        pb.title("Vignette");
        pb.border(border_style::rounded);
        pb.shader(shaders::vignette(0.5f));
        con.print_widget(padding_builder(std::move(pb)).pad(0, 2));
    }
}

// ════════════════════════════════════════════════════════════════════════
// 16. Color Downgrade
// ════════════════════════════════════════════════════════════════════════

static void demo_color_downgrade(console &con) {
    section_header(con, "16", "Color Downgrade");

    con.print("  [bold]color::downgrade(depth)[/] converts RGB to lower color depths:");
    con.newline();

    struct entry {
        const char *name;
        uint8_t r, g, b;
    };
    entry colors_list[] = {
        {"Red", 255, 0, 0}, {"Green", 0, 255, 0}, {"Blue", 0, 0, 255}, {"Orange", 255, 128, 0}, {"Cyan", 0, 255, 255},
    };

    table_builder tb;
    tb.add_column("Color", {justify::left, 10, 14});
    tb.add_column("Original", {justify::center, 14, 18});
    tb.add_column("-> 256", {justify::center, 12, 16});
    tb.add_column("-> 16", {justify::center, 12, 16});
    tb.add_column("-> none", {justify::center, 10, 14});
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});
    tb.border(border_style::rounded);

    for (auto &e : colors_list) {
        auto c = color::from_rgb(e.r, e.g, e.b);
        auto d256 = c.downgrade(2);
        auto d16 = c.downgrade(1);
        auto d0 = c.downgrade(0);

        char orig[24], s256[24], s16[24], s0[24];
        std::snprintf(orig, sizeof(orig), "(%u,%u,%u)", e.r, e.g, e.b);
        if (d256.kind == color_kind::indexed_256) {
            std::snprintf(s256, sizeof(s256), "idx %u", d256.r);
        } else {
            std::snprintf(s256, sizeof(s256), "(%u,%u,%u)", d256.r, d256.g, d256.b);
        }
        if (d16.kind == color_kind::indexed_16) {
            std::snprintf(s16, sizeof(s16), "idx %u", d16.r);
        } else {
            std::snprintf(s16, sizeof(s16), "-");
        }
        std::snprintf(s0, sizeof(s0), "%s", d0.is_default() ? "stripped" : "?");

        tb.add_row({e.name, orig, s256, s16, s0});
    }

    con.print_widget(padding_builder(std::move(tb)).pad(0, 2));
}

// ════════════════════════════════════════════════════════════════════════
// 17. OSC Sequences
// ════════════════════════════════════════════════════════════════════════

static void demo_osc_sequences(console &con) {
    section_header(con, "17", "OSC Sequences (Operating System Commands)");

    con.print("  [bold]Available OSC builders in tapiru::ansi namespace:[/]");
    con.newline();

    table_builder tb;
    tb.add_column("Function", {justify::left, 28, 36});
    tb.add_column("OSC", {justify::center, 8, 10});
    tb.add_column("Description", {justify::left, 30, 50});
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});
    tb.border(border_style::rounded);

    tb.add_row({"set_title(text)", "0", "Set window title + icon"});
    tb.add_row({"set_icon_name(name)", "1", "Set icon name only"});
    tb.add_row({"set_window_title(text)", "2", "Set window title only"});
    tb.add_row({"set_palette_color(i,r,g,b)", "4", "Set 256-palette entry"});
    tb.add_row({"query_palette_color(i)", "4", "Query palette entry"});
    tb.add_row({"reset_palette_color(i)", "104", "Reset palette entry"});
    tb.add_row({"report_cwd(host, path)", "7", "Report working directory"});
    tb.add_row({"hyperlink_open(url, id)", "8", "Open clickable hyperlink"});
    tb.add_row({"hyperlink_close", "8", "Close hyperlink"});
    tb.add_row({"notify(message)", "9", "Desktop notification"});
    tb.add_row({"set_foreground_color(r,g,b)", "10", "Set terminal fg color"});
    tb.add_row({"set_background_color(r,g,b)", "11", "Set terminal bg color"});
    tb.add_row({"set_cursor_color(r,g,b)", "12", "Set cursor color"});
    tb.add_row({"clipboard_write(sel, b64)", "52", "Write to clipboard"});
    tb.add_row({"clipboard_query(sel)", "52", "Query clipboard"});
    tb.add_row({"prompt_start", "133", "Shell integration: prompt"});
    tb.add_row({"command_start", "133", "Shell integration: cmd"});
    tb.add_row({"command_finished(code)", "133", "Shell integration: done"});

    con.print_widget(padding_builder(std::move(tb)).pad(0, 2));

    // Live hyperlink demo
    con.print("  [bold]Live hyperlink demo:[/]");
    {
        std::string out = "    ";
        out += ansi::hyperlink_open("https://github.com");
        out += "\033[4;94m"; // underline bright blue
        out += "Click here to visit GitHub";
        out += "\033[0m";
        out += ansi::hyperlink_close;
        con.write(out);
        con.newline();
    }
}

// ════════════════════════════════════════════════════════════════════════
// 18. Low-level ANSI
// ════════════════════════════════════════════════════════════════════════

static void demo_low_level_ansi(console &con) {
    section_header(con, "18", "Low-level ANSI Utilities");

    con.print("  [bold]Cursor control:[/]");
    con.print("    [cyan]ansi::cursor_to(row, col)[/]  — absolute positioning");
    con.print("    [cyan]ansi::cursor_up/down/left/right(n)[/]  — relative movement");
    con.print("    [cyan]ansi::cursor_hide / cursor_show[/]  — visibility toggle");
    con.newline();

    con.print("  [bold]Screen control:[/]");
    con.print("    [cyan]ansi::clear_screen[/]  — clear entire screen");
    con.print("    [cyan]ansi::clear_to_end[/]  — clear to end of screen");
    con.print("    [cyan]ansi::clear_line[/]    — clear entire line");
    con.newline();

    con.print("  [bold]SGR builder:[/]");
    con.print("    [cyan]ansi::sgr(style)[/]  — build complete SGR string from style");
    con.print("    [cyan]ansi::append_fg_params(out, color)[/]  — append fg SGR params");
    con.print("    [cyan]ansi::append_bg_params(out, color)[/]  — append bg SGR params");
    con.print("    [cyan]ansi::append_attr_params(out, attr, sep)[/]  — append attr SGR params");

    // Show what sgr() produces
    con.newline();
    con.print("  [bold]Example: ansi::sgr() output:[/]");
    {
        style s1{colors::red, {}, attr::bold};
        style s2{colors::green, colors::blue, attr::italic | attr::underline};
        auto sgr1 = ansi::sgr(s1);
        auto sgr2 = ansi::sgr(s2);

        // Escape the ESC character for display
        auto escape_esc = [](const std::string &s) -> std::string {
            std::string out;
            for (char c : s) {
                if (c == '\033') {
                    out += "\\033";
                } else {
                    out += c;
                }
            }
            return out;
        };

        con.print(std::string("    bold red       -> [code_inline]") + escape_esc(sgr1) + "[/]");
        con.print(std::string("    italic ul g/b  -> [code_inline]") + escape_esc(sgr2) + "[/]");
    }
}

// ════════════════════════════════════════════════════════════════════════
//  Grand Finale
// ════════════════════════════════════════════════════════════════════════

static void demo_finale(console &con) {
    con.newline();
    con.print_widget(rule_builder("[bold bright_white]Grand Finale[/]")
                         .rule_style(style{colors::bright_cyan, {}, attr::bold})
                         .gradient(linear_gradient{color::from_rgb(255, 0, 128), color::from_rgb(0, 128, 255),
                                                   gradient_direction::horizontal}));
    con.newline();

    // Everything combined in one layout
    {
        columns_builder summary_cols;
        {
            rows_builder c1;
            c1.add(text_builder("[bold cyan]Core[/]"));
            c1.add(text_builder("  [green]\xe2\x9c\x93[/] Style system"));
            c1.add(text_builder("  [green]\xe2\x9c\x93[/] ANSI emitter"));
            c1.add(text_builder("  [green]\xe2\x9c\x93[/] Terminal detect"));
            c1.add(text_builder("  [green]\xe2\x9c\x93[/] Color downgrade"));
            c1.gap(0);
            summary_cols.add(std::move(c1), 1);
        }
        {
            rows_builder c2;
            c2.add(text_builder("[bold cyan]Markup[/]"));
            c2.add(text_builder("  [green]\xe2\x9c\x93[/] Inline tags"));
            c2.add(text_builder("  [green]\xe2\x9c\x93[/] Block tags"));
            c2.add(text_builder("  [green]\xe2\x9c\x93[/] Constexpr parse"));
            c2.add(text_builder("  [green]\xe2\x9c\x93[/] Semantic aliases"));
            c2.gap(0);
            summary_cols.add(std::move(c2), 1);
        }
        {
            rows_builder c3;
            c3.add(text_builder("[bold cyan]Widgets[/]"));
            c3.add(text_builder("  [green]\xe2\x9c\x93[/] Panel / Table"));
            c3.add(text_builder("  [green]\xe2\x9c\x93[/] Columns / Rows"));
            c3.add(text_builder("  [green]\xe2\x9c\x93[/] Rule / Text"));
            c3.add(text_builder("  [green]\xe2\x9c\x93[/] Overlay / Center"));
            c3.gap(0);
            summary_cols.add(std::move(c3), 1);
        }
        {
            rows_builder c4;
            c4.add(text_builder("[bold cyan]Effects[/]"));
            c4.add(text_builder("  [green]\xe2\x9c\x93[/] Gradients"));
            c4.add(text_builder("  [green]\xe2\x9c\x93[/] Shaders"));
            c4.add(text_builder("  [green]\xe2\x9c\x93[/] Shadows"));
            c4.add(text_builder("  [green]\xe2\x9c\x93[/] OSC sequences"));
            c4.gap(0);
            summary_cols.add(std::move(c4), 1);
        }
        summary_cols.gap(1);

        rows_builder body;
        body.add(text_builder("[bold bright_white]tapiru[/] [dim]\xe2\x80\x94 Modern C++23 Terminal UI Library[/]")
                     .align(justify::center));
        body.add(rule_builder().gradient(linear_gradient{color::from_rgb(255, 100, 0), color::from_rgb(0, 100, 255),
                                                         gradient_direction::horizontal}));
        body.add(std::move(summary_cols));
        body.gap(1);

        panel_builder finale_pb(std::move(body));
        finale_pb.title("Feature Summary");
        finale_pb.border(border_style::double_);
        finale_pb.border_style_override(style{colors::bright_cyan});
        finale_pb.title_style(style{colors::bright_white, {}, attr::bold});
        con.print_widget(std::move(finale_pb));
    }

    con.newline();
    con.print("  [dim]All features demonstrated. Run with a modern terminal for best results.[/]");
    con.newline();
}

// ════════════════════════════════════════════════════════════════════════
//  main
// ════════════════════════════════════════════════════════════════════════

#ifdef _WIN32
static int crt_report_hook(int reportType, char *message, int *returnValue) {
    const char *type_str = "UNKNOWN";
    if (reportType == _CRT_WARN) {
        type_str = "WARNING";
    }
    if (reportType == _CRT_ERROR) {
        type_str = "ERROR";
    }
    if (reportType == _CRT_ASSERT) {
        type_str = "ASSERT";
    }
    std::fprintf(stderr, "\n*** CRT %s: %s\n", type_str, message ? message : "(null)");
    std::fflush(stderr);
    *returnValue = 0; // don't trigger debugger break
    if (reportType == _CRT_ASSERT) {
        std::_Exit(3);
    }
    return 1; // handled, don't show dialog
}
#endif

static void abort_handler(int sig) {
    std::fprintf(stderr, "\n*** SIGNAL %d (abort/segfault)\n", sig);
    std::fflush(stderr);
    std::_Exit(3);
}

#ifdef _WIN32
static LONG WINAPI vectored_handler(EXCEPTION_POINTERS *ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    void *addr = ep->ExceptionRecord->ExceptionAddress;
    const char *desc = "unknown";
    if (code == EXCEPTION_ACCESS_VIOLATION) {
        desc = "ACCESS_VIOLATION";
    } else if (code == EXCEPTION_STACK_OVERFLOW) {
        desc = "STACK_OVERFLOW";
    } else if (code == EXCEPTION_INT_DIVIDE_BY_ZERO) {
        desc = "INT_DIVIDE_BY_ZERO";
    } else if (code == EXCEPTION_BREAKPOINT) {
        desc = "BREAKPOINT";
    } else {
        return EXCEPTION_CONTINUE_SEARCH; // not ours
    }

    std::fprintf(stderr, "\n*** SEH EXCEPTION 0x%08lX (%s) at %p\n", code, desc, addr);
    std::fflush(stderr);
    std::_Exit(3);
}

static void invalid_param_handler(const wchar_t *expression, const wchar_t *function, const wchar_t *file,
                                  unsigned int line, uintptr_t) {
    std::fprintf(stderr, "\n*** CRT INVALID PARAMETER:\n");
    if (expression) {
        std::fwprintf(stderr, L"  Expression: %s\n", expression);
    }
    if (function) {
        std::fwprintf(stderr, L"  Function:   %s\n", function);
    }
    if (file) {
        std::fwprintf(stderr, L"  File:       %s (line %u)\n", file, line);
    }
    std::fflush(stderr);
    std::_Exit(3);
}
#endif

static void install_crash_handlers() {
#ifdef _WIN32
    // Suppress all CRT assertion/error dialogs
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportHook(crt_report_hook);
    // Disable Windows Error Reporting dialog
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    // Disable invalid parameter handler dialog
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    // Catch invalid CRT parameters (e.g. bad format strings, bad fd)
    _set_invalid_parameter_handler(invalid_param_handler);
    // Vectored exception handler for access violations etc.
    AddVectoredExceptionHandler(1, vectored_handler);
#endif
    std::signal(SIGABRT, abort_handler);
    std::signal(SIGSEGV, abort_handler);
}

int main() {
    install_crash_handlers();

    terminal::enable_vt_processing();
    terminal::enable_utf8();

    console con;

    con.newline();
    {
        panel_builder title_pb(text_builder("[bold bright_white]tapiru Feature Showcase[/]").align(justify::center));
        title_pb.border(border_style::double_);
        title_pb.border_style_override(style{colors::bright_cyan, {}, attr::bold});
        con.print_widget(std::move(title_pb));
    }
    con.print("  [dim]A non-interactive tour of every tapiru feature[/]");

    demo_terminal_detection(con);
    demo_style_system(con);
    demo_ansi_emitter(con);
    demo_inline_markup(con);
    demo_constexpr_markup(con);
    demo_rich_markup_runtime(con);
    demo_rich_markup_constexpr(con);
    demo_text_builder(con);
    demo_rule_builder(con);
    demo_panel_builder(con);
    demo_table_builder(con);
    demo_layout_builders(con);
    demo_utility_builders(con);
    demo_gradients(con);
    demo_shaders(con);
    demo_color_downgrade(con);
    demo_osc_sequences(con);
    demo_low_level_ansi(con);
    demo_finale(con);

    return 0;
}
