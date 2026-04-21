/**
 * @file stage5_7_demo.cpp
 * @brief Focused demo for Stage 5-7 features: canvas, gauge, paragraph,
 *        element pipes, color downgrade, double underline.
 *
 * Build:
 *   cmake --build build --target tapiru_stage5_7_demo --config Debug
 * Run:
 *   .\build\bin\Debug\tapiru_stage5_7_demo.exe
 */

#include <cmath>
#include <cstdio>
#include <string>

// ── Core ────────────────────────────────────────────────────────────────
#include "tapiru/core/console.h"
#include "tapiru/core/decorator.h"
#include "tapiru/core/element.h"
#include "tapiru/core/style.h"
#include "tapiru/core/terminal.h"

// ── Widgets ─────────────────────────────────────────────────────────────
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/canvas_widget.h"
#include "tapiru/widgets/gauge.h"
#include "tapiru/widgets/paragraph.h"

using namespace tapiru;

static constexpr uint32_t W = 72;

static void section(console &con, const char *title) {
    con.newline();
    con.print_widget(rule_builder(title).rule_style(style{colors::bright_cyan, {}, attr::bold}), W);
    con.newline();
}

// ═══════════════════════════════════════════════════════════════════════
//  1. Element Pipe Composition (Stage 1 — showcased here)
// ═══════════════════════════════════════════════════════════════════════

static void demo_pipes(console &con) {
    section(con, " 1. Element | Decorator Pipe Composition ");

    con.print("[dim]Elements compose via operator|: elem | border() | bold() | fg_color()[/]");
    con.newline();

    // 1a. Simple border + bold
    auto e1 = element(text_builder("Simple: text | border | bold")) | border() | bold();
    con.print_widget(e1, 45);
    con.newline();

    // 1b. Padding + rounded border + color
    auto e2 = element(text_builder("Padded + colored")) | padding(1, 3) | border(border_style::rounded) |
              fg_color(colors::bright_green);
    con.print_widget(e2, 40);
    con.newline();

    // 1c. Centered with heavy border
    auto e3 = element(text_builder("[bold bright_yellow]Centered & Heavy[/]")) | border(border_style::heavy) | center();
    con.print_widget(e3, W);
    con.newline();

    // 1d. Nested decorators: italic + dim + underline
    auto e4 = element(text_builder("italic + dim + underline")) | italic() | dim() | underline() |
              border(border_style::ascii);
    con.print_widget(e4, 40);
    con.newline();

    // 1e. Size constraints
    auto e5 = element(text_builder("[bold]Fixed 30x3[/]")) | size(30, 3) | border(border_style::rounded);
    con.print_widget(e5, W);
}

// ═══════════════════════════════════════════════════════════════════════
//  2. Canvas Drawing API (Stage 5)
// ═══════════════════════════════════════════════════════════════════════

static void demo_canvas(console &con) {
    section(con, " 2. Canvas Drawing API (Braille + Block) ");

    // 2a. Geometric shapes
    con.print("[dim]2a. Geometric shapes — rect, diagonals, circle, points:[/]");
    {
        auto cvs = make_canvas(64, 32, [](canvas_widget_builder &c) {
            c.draw_rect(0, 0, 63, 31, colors::bright_black);
            c.draw_line(2, 2, 61, 29, colors::green);
            c.draw_line(61, 2, 2, 29, colors::red);
            c.draw_circle(32, 16, 14, colors::bright_cyan);
            c.draw_circle(32, 16, 8, colors::bright_yellow);
            for (int i = 0; i < 64; i += 4) {
                c.draw_point(i, 0, colors::magenta);
                c.draw_point(i, 31, colors::magenta);
            }
        });
        con.print_widget(cvs, W);
    }
    con.newline();

    // 2b. Function plots
    con.print("[dim]2b. Function plots — sin(x) and cos(x):[/]");
    {
        auto cvs = make_canvas(80, 28, [](canvas_widget_builder &c) {
            // Axes
            c.draw_line(0, 14, 79, 14, colors::bright_black);
            c.draw_line(0, 0, 0, 27, colors::bright_black);

            // sin(x) in green
            for (int x = 1; x < 80; ++x) {
                int y = 14 + static_cast<int>(12.0 * std::sin(x * 0.12));
                if (y >= 0 && y < 28) {
                    c.draw_point(x, y, colors::bright_green);
                }
            }
            // cos(x) in cyan
            for (int x = 1; x < 80; ++x) {
                int y = 14 + static_cast<int>(12.0 * std::cos(x * 0.12));
                if (y >= 0 && y < 28) {
                    c.draw_point(x, y, colors::bright_cyan);
                }
            }

            c.draw_text(2, 1, "sin(x)", style{colors::bright_green, {}, attr::bold});
            c.draw_text(2, 3, "cos(x)", style{colors::bright_cyan, {}, attr::bold});
        });
        con.print_widget(cvs, W);
    }
    con.newline();

    // 2c. Concentric circles
    con.print("[dim]2c. Concentric circles:[/]");
    {
        auto cvs = make_canvas(60, 30, [](canvas_widget_builder &c) {
            const color ring_colors[] = {colors::red,  colors::yellow, colors::green,
                                         colors::cyan, colors::blue,   colors::magenta};
            for (int r = 3; r <= 14; r += 2) {
                int idx = (r / 2) % 6;
                c.draw_circle(30, 15, r, ring_colors[idx]);
            }
        });
        con.print_widget(cvs, W);
    }
    con.newline();

    // 2d. Block-mode lines (half-block characters)
    con.print("[dim]2d. Block-mode lines (half-block characters):[/]");
    {
        canvas_widget_builder cvs(50, 20);
        cvs.draw_block_line(0, 0, 49, 19, colors::bright_magenta);
        cvs.draw_block_line(0, 19, 49, 0, colors::bright_blue);
        cvs.draw_block_line(25, 0, 25, 19, colors::bright_yellow);
        cvs.draw_block_line(0, 10, 49, 10, colors::bright_green);
        con.print_widget(std::move(cvs), W);
    }
    con.newline();

    // 2e. Canvas in a panel
    con.print("[dim]2e. Canvas inside a bordered panel:[/]");
    {
        auto cvs = make_canvas(40, 16, [](canvas_widget_builder &c) {
            // Star pattern
            c.draw_line(20, 0, 10, 15, colors::bright_yellow);
            c.draw_line(20, 0, 30, 15, colors::bright_yellow);
            c.draw_line(0, 5, 39, 5, colors::bright_yellow);
            c.draw_line(0, 5, 30, 15, colors::bright_yellow);
            c.draw_line(39, 5, 10, 15, colors::bright_yellow);
        });
        panel_builder pb(std::move(cvs));
        pb.title("Star Pattern");
        pb.border(border_style::rounded);
        con.print_widget(std::move(pb), 30);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  3. Gauge Widget (Stage 6)
// ═══════════════════════════════════════════════════════════════════════

static void demo_gauge(console &con) {
    section(con, " 3. Gauge Widget (Block Characters) ");

    // 3a. Progress levels
    con.print("[dim]3a. Horizontal gauges at 0%, 25%, 50%, 75%, 100%:[/]");
    {
        rows_builder rb;
        const float levels[] = {0.0f, 0.25f, 0.50f, 0.75f, 1.0f};
        for (float p : levels) {
            columns_builder row;
            char label[16];
            std::snprintf(label, sizeof(label), "%3d%%", static_cast<int>(p * 100));
            row.add(text_builder(label));
            row.add(make_gauge(p), 1);
            row.gap(1);
            rb.add(std::move(row));
        }
        rb.gap(0);
        con.print_widget(std::move(rb), 55);
    }
    con.newline();

    // 3b. Custom styled gauge
    con.print("[dim]3b. Custom-styled gauges:[/]");
    {
        rows_builder rb;

        // Green fill
        {
            columns_builder row;
            row.add(text_builder("[green]Health[/] "));
            row.add(make_gauge(0.85f, style{colors::bright_green, {}, attr::bold},
                               style{colors::bright_black, {}, attr::dim}),
                    1);
            row.gap(1);
            rb.add(std::move(row));
        }
        // Red fill
        {
            columns_builder row;
            row.add(text_builder("[red]Danger[/] "));
            row.add(make_gauge(0.20f, style{colors::bright_red, {}, attr::bold},
                               style{colors::bright_black, {}, attr::dim}),
                    1);
            row.gap(1);
            rb.add(std::move(row));
        }
        // Yellow fill
        {
            columns_builder row;
            row.add(text_builder("[yellow]Warn  [/] "));
            row.add(make_gauge(0.55f, style{colors::bright_yellow, {}, attr::bold},
                               style{colors::bright_black, {}, attr::dim}),
                    1);
            row.gap(1);
            rb.add(std::move(row));
        }

        rb.gap(0);
        con.print_widget(std::move(rb), 55);
    }
    con.newline();

    // 3c. Vertical gauges side by side
    con.print("[dim]3c. Vertical gauges (30%, 60%, 90%):[/]");
    {
        columns_builder cols;
        cols.add(make_gauge_direction(0.3f, gauge_direction::vertical));
        cols.add(make_gauge_direction(0.6f, gauge_direction::vertical));
        cols.add(make_gauge_direction(0.9f, gauge_direction::vertical));
        cols.gap(3);
        con.print_widget(std::move(cols), 20);
    }
    con.newline();

    // 3d. Dashboard panel with gauges
    con.print("[dim]3d. System monitor dashboard:[/]");
    {
        rows_builder inner;
        inner.add(text_builder("[bold bright_white]CPU Usage[/]"));
        inner.add(make_gauge(0.73f));
        inner.add(text_builder("[bold bright_white]Memory[/]"));
        inner.add(make_gauge(0.45f));
        inner.add(text_builder("[bold bright_white]Disk I/O[/]"));
        inner.add(make_gauge(0.12f));
        inner.add(text_builder("[bold bright_white]Network[/]"));
        inner.add(make_gauge(0.88f));
        inner.gap(0);

        panel_builder pb(std::move(inner));
        pb.title("System Monitor");
        pb.border(border_style::rounded);
        pb.border_style_override(style{colors::bright_cyan});
        con.print_widget(std::move(pb), 55);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  4. Paragraph Widget (Stage 6)
// ═══════════════════════════════════════════════════════════════════════

static void demo_paragraph(console &con) {
    section(con, " 4. Paragraph Widget (Word Wrap) ");

    // 4a. Basic word wrap
    con.print("[dim]4a. Automatic word wrapping at narrow width:[/]");
    {
        auto para = make_paragraph("The quick brown fox jumps over the lazy dog. "
                                   "This sentence demonstrates automatic word wrapping in the tapiru "
                                   "paragraph widget. Long text is broken at word boundaries to fit "
                                   "within the available width, producing clean multi-line output.");
        panel_builder pb(std::move(para));
        pb.border(border_style::rounded);
        pb.title("Word Wrap Demo");
        con.print_widget(std::move(pb), 42);
    }
    con.newline();

    // 4b. Justified text
    con.print("[dim]4b. Justified paragraph (even spacing):[/]");
    {
        auto para = make_paragraph_justify("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                                           "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                                           "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
                                           "nisi ut aliquip ex ea commodo consequat.");
        panel_builder pb(std::move(para));
        pb.border(border_style::rounded);
        pb.title("Justified Text");
        con.print_widget(std::move(pb), 55);
    }
    con.newline();

    // 4c. Newline preservation
    con.print("[dim]4c. Explicit newlines preserved:[/]");
    {
        auto para = make_paragraph("Chapter 1: Introduction\n"
                                   "The story begins on a dark and stormy night.\n"
                                   "\n"
                                   "Chapter 2: The Journey\n"
                                   "Our hero sets out on an epic adventure.");
        con.print_widget(para, 50);
    }
    con.newline();

    // 4d. Paragraph in a two-column layout
    con.print("[dim]4d. Two-column article layout:[/]");
    {
        columns_builder cols;

        auto left = make_paragraph("The tapiru library provides a comprehensive set of widgets "
                                   "for building rich terminal user interfaces in C++23.");
        panel_builder lp(std::move(left));
        lp.title("About");
        lp.border(border_style::rounded);
        cols.add(std::move(lp), 1);

        auto right = make_paragraph("Features include braille canvas drawing, progress gauges, "
                                    "word-wrapping paragraphs, and color depth downgrade support.");
        panel_builder rp(std::move(right));
        rp.title("Features");
        rp.border(border_style::rounded);
        cols.add(std::move(rp), 1);

        cols.gap(1);
        con.print_widget(std::move(cols), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  5. Color Downgrade (Stage 7)
// ═══════════════════════════════════════════════════════════════════════

static void demo_color_downgrade(console &con) {
    section(con, " 5. Color Downgrade Algorithm ");

    con.print("[dim]color::downgrade(depth) converts RGB → 256 → 16 → none:[/]");
    con.newline();

    // Downgrade chain table
    table_builder tb;
    tb.add_column("Color", {justify::left, 20, 25});
    tb.add_column("RGB", {justify::center, 14, 18});
    tb.add_column("→ 256", {justify::center, 8, 12});
    tb.add_column("→ 16", {justify::center, 8, 12});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});

    auto add_color = [&](const char *name, uint8_t r, uint8_t g, uint8_t b) {
        auto c = color::from_rgb(r, g, b);
        auto d256 = c.downgrade(2);
        auto d16 = c.downgrade(1);
        char rgb_str[32], s256[16], s16[16];
        std::snprintf(rgb_str, sizeof(rgb_str), "(%u, %u, %u)", r, g, b);
        std::snprintf(s256, sizeof(s256), "%u", d256.r);
        std::snprintf(s16, sizeof(s16), "%u", d16.r);
        tb.add_row({name, rgb_str, s256, s16});
    };

    add_color("[red]Pure Red[/]", 255, 0, 0);
    add_color("[green]Pure Green[/]", 0, 255, 0);
    add_color("[blue]Pure Blue[/]", 0, 0, 255);
    add_color("[#FF8000]Orange[/]", 255, 128, 0);
    add_color("[#FF00FF]Magenta[/]", 255, 0, 255);
    add_color("[cyan]Cyan[/]", 0, 255, 255);
    add_color("[dim]Gray(128)[/]", 128, 128, 128);
    add_color("[dim]Gray(64)[/]", 64, 64, 64);
    add_color("[bright_white]White[/]", 255, 255, 255);
    add_color("Black", 0, 0, 0);

    con.print_widget(tb, W);
    con.newline();

    // Show downgrade chain visually
    con.print("[dim]Downgrade chain for orange RGB(255,100,0):[/]");
    {
        auto c = color::from_rgb(255, 100, 0);
        auto d256 = c.downgrade(2);
        auto d16 = c.downgrade(1);
        auto d0 = c.downgrade(0);

        rows_builder rb;
        char buf[128];
        std::snprintf(buf, sizeof(buf), "[#FF6400]  RGB(255,100,0)[/]  kind=rgb");
        rb.add(text_builder(buf));
        std::snprintf(buf, sizeof(buf), "  → 256-color index %u  kind=indexed_256", d256.r);
        rb.add(text_builder(buf));
        std::snprintf(buf, sizeof(buf), "  → 16-color index %u   kind=indexed_16", d16.r);
        rb.add(text_builder(buf));
        std::snprintf(buf, sizeof(buf), "  → %s              kind=default", d0.is_default() ? "stripped" : "???");
        rb.add(text_builder(buf));
        rb.gap(0);
        con.print_widget(std::move(rb), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  6. Double Underline & Attribute Bits (Stage 7)
// ═══════════════════════════════════════════════════════════════════════

static void demo_attributes(console &con) {
    section(con, " 6. Text Attributes (incl. double_underline) ");

    // Attribute reference table
    table_builder tb;
    tb.add_column("Attribute", {justify::left, 22, 28});
    tb.add_column("Bit", {justify::center, 8, 12});
    tb.add_column("SGR Code", {justify::center, 10, 14});
    tb.border(border_style::rounded);
    tb.header_style(style{colors::bright_cyan, {}, attr::bold});

    tb.add_row({"[bold]bold[/]", "0x0001", "SGR 1"});
    tb.add_row({"[dim]dim[/]", "0x0002", "SGR 2"});
    tb.add_row({"[italic]italic[/]", "0x0004", "SGR 3"});
    tb.add_row({"[underline]underline[/]", "0x0008", "SGR 4"});
    tb.add_row({"blink", "0x0010", "SGR 5"});
    tb.add_row({"[reverse]reverse[/]", "0x0020", "SGR 7"});
    tb.add_row({"hidden", "0x0040", "SGR 8"});
    tb.add_row({"[strike]strikethrough[/]", "0x0080", "SGR 9"});
    tb.add_row({"[bold cyan]double_underline[/] (new!)", "0x0100", "SGR 21"});

    con.print_widget(tb, 60);
    con.newline();

    // Demonstrate combining double_underline
    con.print("[dim]Combining double_underline with other attributes:[/]");
    {
        rows_builder rb;

        {
            style s;
            s.attrs = attr::double_underline;
            s.fg = colors::bright_white;
            auto t = text_builder("  double_underline only");
            t.style_override(s);
            rb.add(std::move(t));
        }
        {
            style s;
            s.attrs = attr::double_underline | attr::bold;
            s.fg = colors::bright_cyan;
            auto t = text_builder("  double_underline + bold");
            t.style_override(s);
            rb.add(std::move(t));
        }
        {
            style s;
            s.attrs = attr::double_underline | attr::italic;
            s.fg = colors::bright_green;
            auto t = text_builder("  double_underline + italic");
            t.style_override(s);
            rb.add(std::move(t));
        }
        {
            style s;
            s.attrs = attr::double_underline | attr::bold | attr::italic;
            s.fg = colors::bright_yellow;
            auto t = text_builder("  double_underline + bold + italic");
            t.style_override(s);
            rb.add(std::move(t));
        }

        rb.gap(0);
        con.print_widget(std::move(rb), W);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  7. Combined Showcase
// ═══════════════════════════════════════════════════════════════════════

static void demo_combined(console &con) {
    section(con, " 7. Combined Showcase ");

    con.print("[dim]All Stage 5-7 features in a single dashboard:[/]");
    con.newline();

    rows_builder dashboard;

    // Title bar
    dashboard.add(text_builder("[bold bright_white on_blue]  tapiru Stage 5-7 Feature Dashboard  [/]"));

    // Body: canvas + gauges side by side
    {
        columns_builder body;

        // Left: mini canvas
        {
            auto cvs = make_canvas(40, 20, [](canvas_widget_builder &c) {
                c.draw_rect(0, 0, 39, 19, colors::bright_black);
                c.draw_circle(20, 10, 8, colors::bright_cyan);
                for (int x = 0; x < 40; ++x) {
                    int y = 10 + static_cast<int>(8.0 * std::sin(x * 0.3));
                    if (y >= 0 && y < 20) {
                        c.draw_point(x, y, colors::bright_green);
                    }
                }
            });
            panel_builder pb(std::move(cvs));
            pb.title("Canvas");
            pb.border(border_style::rounded);
            body.add(std::move(pb), 1);
        }

        // Right: gauges + paragraph
        {
            rows_builder right;

            // Gauges
            right.add(text_builder("[bold]System Status[/]"));
            right.add(make_gauge(0.82f));
            right.add(make_gauge(0.45f));
            right.add(make_gauge(0.67f));

            // Mini paragraph
            right.add(text_builder(""));
            auto para = make_paragraph("All systems operational. Canvas rendering, gauge widgets, "
                                       "and paragraph wrapping working correctly.");
            right.add(std::move(para));
            right.gap(0);

            panel_builder pb(std::move(right));
            pb.title("Status");
            pb.border(border_style::rounded);
            body.add(std::move(pb), 1);
        }

        body.gap(1);
        dashboard.add(std::move(body));
    }

    dashboard.gap(0);
    con.print_widget(std::move(dashboard), W);
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    console con;

    con.newline();
    con.print_widget(rule_builder(" tapiru — Stage 5-7 Features Demo ")
                         .rule_style(style{colors::bright_yellow, {}, attr::bold})
                         .character(U'\x2550'),
                     W);

    con.newline();
    con.print("[dim]Stage 5: Canvas drawing API (braille + block characters)[/]");
    con.print("[dim]Stage 6: Gauge widget + paragraph word-wrap + flexbox enums[/]");
    con.print("[dim]Stage 7: Color downgrade + double underline attribute[/]");

    demo_pipes(con);
    demo_canvas(con);
    demo_gauge(con);
    demo_paragraph(con);
    demo_color_downgrade(con);
    demo_attributes(con);
    demo_combined(con);

    con.newline();
    con.print_widget(rule_builder(" Stage 5-7 Demo Complete! ")
                         .rule_style(style{colors::bright_green, {}, attr::bold})
                         .character(U'\x2550'),
                     W);
    con.newline();

    return 0;
}
