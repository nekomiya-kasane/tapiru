/**
 * @file highlighter_demo.cpp
 * @brief Demonstrates the auto-highlight system.
 *
 * Shows built-in repr_highlighter, custom regex_highlighter,
 * highlight chaining, custom themes, and console integration.
 */

#include "tapiru/core/console.h"
#include "tapiru/text/highlighter.h"

#include <string>

static void section(tapiru::console &con, const char *title) {
    con.print("");
    con.print(std::string("[bold bright_yellow]━━━ ") + title + " ━━━[/]");
    con.print("");
}

int main() {
    tapiru::console con;

    con.print("[bold underline]Tapiru Auto-Highlight Demo[/]");

    // ── 1. Built-in repr_highlighter ─────────────────────────────────────
    section(con, "1. Built-in repr_highlighter");

    con.print("[dim]The repr_highlighter automatically detects and styles common patterns.[/]");
    con.print("[dim]Below, each line is plain text — all colors come from auto-highlight.[/]");
    con.print("");

    const auto &repr = tapiru::repr_highlighter::instance();
    con.set_highlighter(&repr);

    con.print_highlighted("Numbers:   42   3.14   0xFF00   0b1010   0o755   1.23e-4   -99");
    con.print_highlighted("Strings:   \"hello world\"   'single quoted'");
    con.print_highlighted("URLs:      https://github.com/user/repo   ftp://files.example.com/data");
    con.print_highlighted("Emails:    admin@example.com   user.name@company.co.uk");
    con.print_highlighted("IPs:       192.168.1.1   10.0.0.255   ::1   fe80::1%25eth0");
    con.print_highlighted("UUIDs:     550e8400-e29b-41d4-a716-446655440000");
    con.print_highlighted("Booleans:  true   false   True   False");
    con.print_highlighted("Nulls:     null   nullptr   None   nil");
    con.print_highlighted("Env vars:  $HOME   ${PATH}   %USERPROFILE%");
    con.print_highlighted("Dates:     2024-01-15   2024-01-15T09:30:00Z");
    con.print_highlighted("Keywords:  TODO fix this   FIXME broken   HACK workaround   BUG crash");
    con.print_highlighted("Paths:     /usr/local/bin/app   C:\\Windows\\System32\\cmd.exe");

    // ── 2. Auto-highlight on print() ─────────────────────────────────────
    section(con, "2. Auto-highlight on print()");

    con.print("[dim]With set_auto_highlight(true), print() auto-highlights plain text[/]");
    con.print("[dim]while preserving explicit markup styling.[/]");
    con.print("");

    con.set_auto_highlight(true);

    con.print("Server started on 192.168.1.100 port 8080 at 2024-06-15T14:30:00Z");
    con.print("Request from user@example.com — status 200 in 42ms");
    con.print("Config loaded from /etc/app/config.json — debug: true, workers: 8");
    con.print("Session id: 550e8400-e29b-41d4-a716-446655440000");

    con.print("");
    con.print("[dim]Markup + auto-highlight coexist — markup takes priority:[/]");
    con.print("");
    con.print("[bold red]ERROR:[/] connection to 192.168.1.1 port 5432 timed out after 30000ms");
    con.print("[bold yellow]WARN:[/]  retrying with fallback at https://backup.example.com/api");
    con.print("[bold green]OK:[/]    loaded 1024 records from /var/data/export.csv");

    con.set_auto_highlight(false);

    // ── 3. Custom regex_highlighter ──────────────────────────────────────
    section(con, "3. Custom regex_highlighter");

    con.print("[dim]Build your own highlighter with custom regex rules.[/]");
    con.print("");

    tapiru::regex_highlighter custom;
    // HTTP methods
    custom.add_word("GET", tapiru::style{tapiru::colors::bright_green, {}, tapiru::attr::bold});
    custom.add_word("POST", tapiru::style{tapiru::colors::bright_yellow, {}, tapiru::attr::bold});
    custom.add_word("PUT", tapiru::style{tapiru::colors::bright_blue, {}, tapiru::attr::bold});
    custom.add_word("DELETE", tapiru::style{tapiru::colors::bright_red, {}, tapiru::attr::bold});
    // Status codes
    custom.add_rule(R"(\b2\d{2}\b)", tapiru::style{tapiru::colors::green});
    custom.add_rule(R"(\b4\d{2}\b)", tapiru::style{tapiru::colors::yellow});
    custom.add_rule(R"(\b5\d{2}\b)", tapiru::style{tapiru::colors::red, {}, tapiru::attr::bold});
    // Quoted paths
    custom.add_rule(R"("/[^"]*")", tapiru::style{tapiru::colors::bright_cyan});
    // Timestamps
    custom.add_rule(R"(\d{2}:\d{2}:\d{2}\.\d{3})", tapiru::style{tapiru::colors::bright_black});

    con.set_highlighter(&custom);

    con.print_highlighted("09:14:22.331  GET    \"/api/users\"       200  12ms");
    con.print_highlighted("09:14:22.587  POST   \"/api/users\"       201  45ms");
    con.print_highlighted("09:14:23.102  GET    \"/api/missing\"     404  3ms");
    con.print_highlighted("09:14:23.998  DELETE \"/api/users/42\"    204  8ms");
    con.print_highlighted("09:14:24.501  PUT    \"/api/config\"      500  102ms");

    // ── 4. Highlight chain ───────────────────────────────────────────────
    section(con, "4. Highlight chain — combine multiple highlighters");

    con.print("[dim]Chain the built-in repr_highlighter with custom rules.[/]");
    con.print("[dim]Earlier highlighters in the chain have priority.[/]");
    con.print("");

    tapiru::regex_highlighter log_levels;
    log_levels.add_word("INFO", tapiru::style{tapiru::colors::black, tapiru::colors::green, tapiru::attr::bold});
    log_levels.add_word("WARN", tapiru::style{tapiru::colors::black, tapiru::colors::yellow, tapiru::attr::bold});
    log_levels.add_word("ERROR", tapiru::style{tapiru::colors::white, tapiru::colors::red, tapiru::attr::bold});
    log_levels.add_word("DEBUG", tapiru::style{tapiru::colors::black, tapiru::colors::bright_blue, tapiru::attr::bold});

    tapiru::highlight_chain chain;
    chain.add(log_levels);                           // log levels first (higher priority)
    chain.add(tapiru::repr_highlighter::instance()); // then general patterns

    con.set_highlighter(&chain);

    con.print_highlighted("2024-06-15T09:30:00Z  INFO   Server listening on 0.0.0.0 port 8080");
    con.print_highlighted("2024-06-15T09:30:01Z  DEBUG  Loading config from /etc/app.json");
    con.print_highlighted("2024-06-15T09:30:02Z  WARN   Connection pool at 90% — max 100");
    con.print_highlighted("2024-06-15T09:30:03Z  ERROR  Failed to reach 10.0.0.5 — timeout 5000ms");
    con.print_highlighted(
        "2024-06-15T09:30:04Z  INFO   User admin@example.com logged in, session 550e8400-e29b-41d4-a716-446655440000");

    // ── 5. Custom theme ──────────────────────────────────────────────────
    section(con, "5. Custom theme for repr_highlighter");

    con.print("[dim]Override default colors with a custom theme.[/]");
    con.print("");

    tapiru::repr_highlighter::theme monokai;
    monokai.number = {tapiru::colors::magenta};
    monokai.number_hex = {tapiru::colors::bright_magenta};
    monokai.string = {tapiru::colors::yellow};
    monokai.url = {tapiru::color::from_rgb(102, 217, 239), {}, tapiru::attr::underline};
    monokai.boolean = {tapiru::colors::magenta, {}, tapiru::attr::italic};
    monokai.null = {tapiru::colors::magenta, {}, tapiru::attr::italic};
    monokai.keyword_todo = {tapiru::color::from_rgb(0, 0, 0), tapiru::color::from_rgb(230, 219, 116),
                            tapiru::attr::bold};

    tapiru::repr_highlighter monokai_hl(monokai);
    con.set_highlighter(&monokai_hl);

    con.print_highlighted("Monokai-ish: count = 42, hex = 0xDEAD, pi = 3.14");
    con.print_highlighted("Monokai-ish: msg = \"hello world\", ok = true, val = null");
    con.print_highlighted("Monokai-ish: url = https://monokai.pro TODO update theme");

    // ── 6. Capture groups ────────────────────────────────────────────────
    section(con, "6. Capture groups — highlight part of a match");

    con.print("[dim]Use capture groups to style only a portion of the regex match.[/]");
    con.print("");

    tapiru::regex_highlighter kv_hl;
    // Highlight only the value (group 2) in key=value pairs
    kv_hl.add_rule(R"((\w+)=(\S+))", tapiru::style{tapiru::colors::bright_green, {}, tapiru::attr::bold}, 2);
    // Highlight only the key (group 1) in key: value pairs
    kv_hl.add_rule(R"((\w+):\s+(\S+))", tapiru::style{tapiru::colors::bright_cyan, {}, tapiru::attr::bold}, 1);

    con.set_highlighter(&kv_hl);

    con.print_highlighted("host=localhost port=8080 workers=4 debug=true");
    con.print_highlighted("name: tapiru   version: 1.0   license: MIT");

    // ── 7. Clickable hyperlinks (OSC 8) ────────────────────────────────
    section(con, "7. Clickable hyperlinks (OSC 8)");

    con.print("[dim]URLs and file paths are automatically wrapped in OSC 8 hyperlinks,[/]");
    con.print("[dim]making them clickable in supported terminals (iTerm2, kitty, WezTerm,[/]");
    con.print("[dim]Windows Terminal, GNOME Terminal, foot, tmux 3.1+, etc.).[/]");
    con.print("");

    con.set_highlighter(&tapiru::repr_highlighter::instance());

    con.print("[bold]Try clicking these links:[/]");
    con.print_highlighted("  Repo:   https://github.com/user/tapiru");
    con.print_highlighted("  Docs:   https://tapiru.dev/docs/highlighter");
    con.print_highlighted("  FTP:    ftp://files.example.com/releases/v1.0.tar.gz");
    con.print("");
    con.print("[bold]File paths become file:// URIs (opens in your file manager/editor):[/]");
    con.print_highlighted("  Config: /etc/app/config.json");
    con.print_highlighted("  Log:    /var/log/app/error.log");
    con.print_highlighted("  Win:    C:\\Users\\dev\\project\\README.md");
    con.print("");
    con.print("[bold]Mixed content — only URLs and paths are clickable:[/]");
    con.print_highlighted("  Server at https://api.example.com returned 200 in 42ms");
    con.print_highlighted("  Loaded 1024 entries from /data/export.csv at 2024-06-15T09:30:00Z");

    // ── Done ─────────────────────────────────────────────────────────────
    con.set_highlighter(nullptr);
    con.print("");
    con.print("[bold bright_green]Demo complete![/]");

    return 0;
}
