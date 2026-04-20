/**
 * @file markup_demo.cpp
 * @brief Demonstrates the extended markup system with block-level tags.
 *
 * Shows compile-time parsed rich markup with boxes, rules, headings,
 * blockquotes, code blocks, lists, progress bars, emoji, and hyperlinks.
 */

#include "tapiru/core/console.h"

int main() {
    tapiru::console con;

    // ── Inline style tags ───────────────────────────────────────────────
    con.print("[bold]Extended Markup Demo[/]");
    con.print("");

    con.print("[bold]1. Inline Tags[/]");
    con.print("  [error]error[/]  [warning]warning[/]  [success]success[/]  [info]info[/]  [debug]debug[/]  [muted]muted[/]");
    con.print("  [highlight]highlight[/]  [code_inline]code_inline[/]  [url]url[/]  [path]path[/]  [key]key[/]");
    con.print("  [badge.red] ERR [/] [badge.green] OK [/] [badge.blue] INFO [/] [badge.yellow] WARN [/]");
    con.print("  [rgb(255,128,0)]rgb(255,128,0)[/]  [on_rgb(0,0,80) white]on_rgb(0,0,80)[/]  [color256(208)]color256(208)[/]");
    con.print("  [overline]overline[/]  [double_underline]double_underline[/]  [superscript]superscript[/]  [subscript]subscript[/]");
    con.print("  [bold italic not_bold]not_bold (italic only)[/]");
    con.print("");

    // ── Block-level tags via TAPIRU_PRINT_RICH ──────────────────────────
    con.print("[bold]2. Block Tags (compile-time parsed)[/]");
    con.print("");

    TAPIRU_PRINT_RICH(con, "[rule]");
    TAPIRU_PRINT_RICH(con, "[rule=Section A]");

    con.print("");
    TAPIRU_PRINT_RICH(con, "[h1]Heading 1[/h1]");
    TAPIRU_PRINT_RICH(con, "[h2]Heading 2[/h2]");
    TAPIRU_PRINT_RICH(con, "[h3]Heading 3[/h3]");

    con.print("");
    TAPIRU_PRINT_RICH(con, "[box]Simple box with rounded borders[/box]");

    con.print("");
    TAPIRU_PRINT_RICH(con, "[box.heavy]Heavy box[/box]");

    con.print("");
    TAPIRU_PRINT_RICH(con, "[quote]This is a blockquote.[/quote]");

    con.print("");
    TAPIRU_PRINT_RICH(con, "[code]int main() { return 0; }[/code]");

    con.print("");
    TAPIRU_PRINT_RICH(con, "[indent=8]Indented by 8 spaces[/indent]");

    con.print("");
    con.print("[bold]3. Progress & Bars[/]");
    TAPIRU_PRINT_RICH(con, "[progress=25]");
    TAPIRU_PRINT_RICH(con, "[progress=50]");
    TAPIRU_PRINT_RICH(con, "[progress=75]");
    TAPIRU_PRINT_RICH(con, "[progress=100]");
    TAPIRU_PRINT_RICH(con, "[bar=3/5]");

    con.print("");
    con.print("[bold]4. Emoji[/]");
    TAPIRU_PRINT_RICH(con, "[emoji=fire] [emoji=rocket] [emoji=star] [emoji=heart] [emoji=check]");

    con.print("");
    con.print("[bold]5. Spacing & Breaks[/]");
    TAPIRU_PRINT_RICH(con, "Line 1[br]Line 2[br][sp=10]Indented line 3");

    con.print("");
    con.print("[bold]6. Mixed inline + block[/]");
    TAPIRU_PRINT_RICH(con, "[bold red]Styled[/] text before [rule=End]");

    return 0;
}
