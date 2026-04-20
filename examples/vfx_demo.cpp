/**
 * @file vfx_demo.cpp
 * @brief Demo showcasing Phase 6-7 visual features: shadows, glow, shaders, widget keys.
 *
 * Build:
 *   cmake --build build --target tapiru_vfx_demo --config Release
 * Run:
 *   .\build\bin\Release\tapiru_vfx_demo.exe
 */

#include "tapiru/core/console.h"
#include "tapiru/core/gradient.h"
#include "tapiru/core/live.h"
#include "tapiru/core/shader.h"
#include "tapiru/core/style.h"
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/spinner.h"

#include <chrono>
#include <cmath>
#include <string>
#include <thread>

using namespace tapiru;

// ═══════════════════════════════════════════════════════════════════════
//  Helper
// ═══════════════════════════════════════════════════════════════════════

static void section(console &con, const char *title) {
    con.newline();
    con.print_widget(rule_builder(title).rule_style(style{colors::bright_cyan, {}, attr::bold}), 70);
    con.newline();
}

// ═══════════════════════════════════════════════════════════════════════
//  1. Box Shadow
// ═══════════════════════════════════════════════════════════════════════

static void demo_shadow(console &con) {
    section(con, " 1. Box Shadow ");

    con.print("[dim]Panel with drop shadow (offset 2,1  blur 1):[/]");
    con.newline();

    panel_builder pb(text_builder("[bold]Hello Shadow![/]\nThis panel casts a soft shadow."));
    pb.title("Shadow Demo");
    pb.border(border_style::rounded);
    pb.shadow(); // default: offset_x=2, offset_y=1, blur=1
    con.print_widget(std::move(pb), 50);

    con.newline();

    con.print("[dim]Heavy shadow (offset 3,2  blur 2  opacity 180):[/]");
    con.newline();

    panel_builder pb2(text_builder("[red]Critical alert![/]\nDeep shadow for emphasis."));
    pb2.title("Heavy Shadow");
    pb2.border(border_style::heavy);
    pb2.border_style_override(style{colors::red, {}, attr::bold});
    pb2.shadow(3, 2, 2, color::from_rgb(80, 0, 0), 180);
    con.print_widget(std::move(pb2), 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  2. Glow Effect
// ═══════════════════════════════════════════════════════════════════════

static void demo_glow(console &con) {
    section(con, " 2. Glow Effect ");

    con.print("[dim]Panel with cyan glow (blur 2, opacity 100):[/]");
    con.newline();

    panel_builder pb(text_builder("[bold cyan]Neon vibes![/]\nGlow emanates from the panel edges."));
    pb.title("Glow");
    pb.border(border_style::rounded);
    pb.border_style_override(style{colors::bright_cyan, {}, attr::bold});
    pb.glow(color::from_rgb(0, 200, 255), 2, 100);
    con.print_widget(std::move(pb), 50);

    con.newline();

    con.print("[dim]Green glow with gradient background:[/]");
    con.newline();

    panel_builder pb2(text_builder("[bold green]System Online[/]\n[dim]All services operational.[/]"));
    pb2.title("Status");
    pb2.border(border_style::rounded);
    pb2.border_style_override(style{colors::bright_green, {}, attr::bold});
    pb2.background_gradient({color::from_rgb(0, 40, 0), color::from_rgb(0, 20, 0)});
    pb2.glow(color::from_rgb(0, 255, 80), 1, 80);
    con.print_widget(std::move(pb2), 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  3. Terminal Shaders (static)
// ═══════════════════════════════════════════════════════════════════════

static void demo_shaders_static(console &con) {
    section(con, " 3. Terminal Shaders (static) ");

    con.print("[dim]Scanline shader (CRT effect):[/]");
    con.newline();

    panel_builder pb1(
        text_builder("[bold]Retro Terminal[/]\n[green]> READY[/]\n[green]> RUN PROGRAM[/]\n[green]> ...[/]"));
    pb1.title("CRT");
    pb1.border(border_style::rounded);
    pb1.border_style_override(style{colors::green, {}});
    pb1.background_gradient({color::from_rgb(0, 20, 0), color::from_rgb(0, 10, 0)});
    pb1.shader(shaders::scanline(0.4f));
    con.print_widget(std::move(pb1), 50);

    con.newline();

    con.print("[dim]Vignette shader (darkened edges):[/]");
    con.newline();

    panel_builder pb2(
        text_builder("[bold white]Cinematic[/]\n\nThe edges fade to darkness,\ndrawing focus to the center."));
    pb2.title("Vignette");
    pb2.border(border_style::rounded);
    pb2.background_gradient({color::from_rgb(40, 20, 60), color::from_rgb(20, 10, 40)});
    pb2.shader(shaders::vignette(0.7f));
    con.print_widget(std::move(pb2), 50);
}

// ═══════════════════════════════════════════════════════════════════════
//  4. Widget Keys
// ═══════════════════════════════════════════════════════════════════════

static void demo_widget_keys(console &con) {
    section(con, " 4. Widget Keys ");

    con.print("[dim]Widgets with identity keys for reconciliation:[/]");
    con.newline();

    columns_builder cols;
    cols.key("main-columns");

    {
        panel_builder pa(text_builder("[bold]Alpha[/]\nkey=\"card-a\""));
        pa.title("Card A")
            .border(border_style::rounded)
            .border_style_override(style{colors::red, {}, attr::bold})
            .key("card-a");
        cols.add(std::move(pa), 1);
    }
    {
        panel_builder pb(text_builder("[bold]Beta[/]\nkey=\"card-b\""));
        pb.title("Card B")
            .border(border_style::rounded)
            .border_style_override(style{colors::green, {}, attr::bold})
            .key("card-b");
        cols.add(std::move(pb), 1);
    }
    {
        panel_builder pc(text_builder("[bold]Gamma[/]\nkey=\"card-c\""));
        pc.title("Card C")
            .border(border_style::rounded)
            .border_style_override(style{colors::blue, {}, attr::bold})
            .key("card-c");
        cols.add(std::move(pc), 1);
    }
    cols.gap(2);

    con.print_widget(std::move(cols), 70);

    con.newline();
    con.print("[dim]Keys enable identity-based reconciliation across frames.[/]");
    con.print("[dim]When widgets reorder, the engine can track them by key.[/]");
}

// ═══════════════════════════════════════════════════════════════════════
//  5. Live Animated Shaders
// ═══════════════════════════════════════════════════════════════════════

static panel_builder make_shimmer_panel(int tick) {
    (void)tick;
    panel_builder pb(text_builder("[bold cyan]Shimmer[/]\nAnimated highlight\nsweeps diagonally\n\n"
                                  "[dim]The shimmer band moves\nacross the panel each frame.[/]"));
    pb.title("Shimmer");
    pb.border(border_style::rounded);
    pb.border_style_override(style{colors::bright_cyan, {}, attr::bold});
    pb.shader(shaders::shimmer(color::from_rgb(255, 255, 200), 1.5f));
    pb.key("shimmer-panel");
    return pb;
}

static panel_builder make_pulse_panel(int tick) {
    (void)tick;
    panel_builder pb(text_builder("[bold magenta]Glow Pulse[/]\nBorder pulses with\na soft glow\n\n"
                                  "[dim]Watch the border edges\npulse rhythmically.[/]"));
    pb.title("Glow Pulse");
    pb.border(border_style::rounded);
    pb.border_style_override(style{colors::bright_magenta, {}, attr::bold});
    pb.shader(shaders::glow_pulse(color::from_rgb(200, 0, 255), 40.0f));
    pb.glow(color::from_rgb(150, 0, 200), 1, 60);
    pb.key("pulse-panel");
    return pb;
}

static void demo_live_shaders(console &con) {
    section(con, " 5. Live Animated Shaders ");

    if (!terminal::is_tty()) {
        con.print("[yellow]Skipped: requires interactive terminal (TTY)[/]");
        return;
    }

    con.print("[dim]Running 4-second live shimmer animation...[/]");
    con.newline();

    {
        live lv(con, 15, 60);
        int tick = 0;
        lv.set(make_shimmer_panel(tick));

        for (int i = 0; i < 30; ++i) {
            lv.set(make_shimmer_panel(++tick));
            std::this_thread::sleep_for(std::chrono::milliseconds(67));
        }
    }

    con.newline();
    con.print("[dim]Running 4-second live glow pulse animation...[/]");
    con.newline();

    {
        live lv(con, 15, 60);
        int tick = 0;
        lv.set(make_pulse_panel(tick));

        for (int i = 0; i < 30; ++i) {
            lv.set(make_pulse_panel(++tick));
            std::this_thread::sleep_for(std::chrono::milliseconds(67));
        }
    }

    con.newline();
    con.print("[bold green]Live shader demo finished![/]");
}

// ═══════════════════════════════════════════════════════════════════════
//  6. Combined: Shadow + Gradient + Shader
// ═══════════════════════════════════════════════════════════════════════

static void demo_combined(console &con) {
    section(con, " 6. Combined Effects ");

    con.print("[dim]Shadow + gradient + scanline shader:[/]");
    con.newline();

    panel_builder pb(text_builder("[bold]Full Stack[/]\n"
                                  "\n"
                                  "This panel combines:\n"
                                  "  [cyan]\xE2\x80\xA2[/] Drop shadow\n"
                                  "  [cyan]\xE2\x80\xA2[/] Gradient background\n"
                                  "  [cyan]\xE2\x80\xA2[/] Scanline shader\n"
                                  "  [cyan]\xE2\x80\xA2[/] Widget key"));
    pb.title("Everything");
    pb.border(border_style::heavy);
    pb.border_style_override(style{colors::bright_yellow, {}, attr::bold});
    pb.background_gradient({color::from_rgb(40, 10, 60), color::from_rgb(10, 10, 40)});
    pb.shadow(2, 1, 1, color::from_rgb(0, 0, 0), 150);
    pb.shader(shaders::scanline(0.25f));
    pb.key("everything-panel");
    con.print_widget(std::move(pb), 55);
}

// ═══════════════════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════════════════

int main() {
    console con;

    con.newline();
    con.print_widget(rule_builder(" tapiru VFX Demo — Phase 6-7 Features ")
                         .rule_style(style{colors::bright_yellow, {}, attr::bold})
                         .character(U'\x2550'), // ═
                     70);

    demo_shadow(con);
    demo_glow(con);
    demo_shaders_static(con);
    demo_widget_keys(con);
    demo_combined(con);
    demo_live_shaders(con);

    con.newline();
    con.print_widget(rule_builder(" VFX Demo Complete! ")
                         .rule_style(style{colors::bright_green, {}, attr::bold})
                         .character(U'\x2550'),
                     70);
    con.newline();

    return 0;
}
