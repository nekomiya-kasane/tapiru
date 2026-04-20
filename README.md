![icon](icon.png)

# tapiru

**Terminal & TUI Library** — Rich-style styled output, composable widget system, live display engine, and tqdm progress bars for modern terminals.

```
tapiru::console con;
con.print("[bold red]Error:[/] something went wrong");
con.print("[green]✓[/] All systems nominal");
```

## Features

- **Rich Markup** — `[bold red]`, `[dim]`, `[on_blue]`, `[#ff8800]` inline styling with automatic ANSI escape generation
- **Widget System** — Composable builders: `text`, `panel`, `table`, `columns`, `rows`, `menu`, `popup`, `overlay`, `spacer`, `rule`
- **Interactive Widgets** — `button`, `checkbox`, `radio_group`, `selectable_list`, `text_input`, `slider`
- **Data Visualization** — `line_chart` (braille), `bar_chart` (block), `scatter` plot, `progress` bar, `spinner`
- **Live Display** — Background render thread with fps control, double-buffered diff-based updates
- **tqdm Iterator** — Python-style progress bars for range-based for loops (header-only)
- **Visual Effects** — Shadows, glows, gradients, alpha compositing, shaders (scanline, shimmer, vignette, glow pulse)
- **Animation** — Easing functions, tween interpolation, `animated<T>` template
- **Event System** — Keyboard/mouse event routing, focus management, hit testing
- **Logging** — Structured log handler with level-based styling
- **Themes** — Named style sheets (dark, light, monokai presets)
- **Text Processing** — Emoji shortcodes, Markdown→markup conversion, constexpr markup parsing
- **Image Rendering** — Half-block pixel art from RGBA data
- **Cross-platform** — Windows (ConPTY/VT) and POSIX, auto-detected color depth (16/256/true color)

## Requirements

- **C++23** compiler (MSVC 17.8+, GCC 13+, Clang 17+)
- **CMake** 4.0+
- No external dependencies (GoogleTest fetched automatically for tests)

## Building

```bash
cmake -S . -B build
cmake --build build --config Release
```

### CMake Options

| Option                  | Default | Description                           |
| ----------------------- | ------- | ------------------------------------- |
| `TAPIRU_BUILD_TESTS`    | `ON`    | Build test suite (fetches GoogleTest) |
| `TAPIRU_BUILD_EXAMPLES` | `ON`    | Build example programs                |

### Integration

```cmake
add_subdirectory(tapiru)
target_link_libraries(your_target PRIVATE tapiru::tapiru)
```

## Quick Start

### Styled Console Output

```cpp
#include "tapiru/core/console.h"

tapiru::console con;

// Markup-styled text
con.print("[bold]Hello[/], [italic green]world[/]!");
con.print("[red on_white]Error:[/] file not found");
con.print("[#ff8800]Custom RGB color[/]");
con.print("[dim underline]Subtle hint[/]");
```

### Widget Rendering

```cpp
#include "tapiru/core/console.h"
#include "tapiru/widgets/builders.h"

using namespace tapiru;

console con;

// Panel with border
auto content = text_builder("[bold]Dashboard[/]\nStatus: [green]Online[/]");
auto panel = panel_builder(std::move(content))
    .title("System")
    .border(border_style::rounded)
    .shadow();
con.print_widget(panel, 60);
```

Output:
```
╭ System ───────────────────────────────────────────────╮
│Dashboard                                              │
│Status: Online                                         │
╰───────────────────────────────────────────────────────╯
```

### Tables

```cpp
table_builder tb;
tb.add_column("Name",   {justify::left,   10, 20});
tb.add_column("Status", {justify::center,  8, 12});
tb.add_column("Score",  {justify::right,   6, 10});
tb.border(border_style::rounded);
tb.header_style(style{colors::bright_cyan, {}, attr::bold});
tb.add_row({"Alice",   "[green]Active[/]",  "98"});
tb.add_row({"Bob",     "[yellow]Idle[/]",    "72"});
tb.add_row({"Charlie", "[red]Offline[/]",    "45"});
tb.shadow();
con.print_widget(tb, 60);
```

### Layouts

```cpp
// Horizontal layout with flex
columns_builder cols;
cols.add(panel_builder(text_builder("Left")), 1);    // flex=1
cols.add(panel_builder(text_builder("Right")), 2);   // flex=2
cols.gap(2);
con.print_widget(cols, 80);

// Vertical layout
rows_builder rows;
rows.add(text_builder("[bold]Header[/]"));
rows.add(rule_builder());
rows.add(text_builder("Body content"), 1);  // flex=1
rows.add(rule_builder());
rows.add(text_builder("[dim]Footer[/]"));
rows.gap(0);
con.print_widget(rows, 80);
```

### Menu & Popup

```cpp
#include "tapiru/widgets/menu.h"
#include "tapiru/widgets/popup.h"

int cursor = 0;
auto menu = menu_builder()
    .add_item("New Project",  "Ctrl+N")
    .add_item("Open Project", "Ctrl+O")
    .add_separator()
    .add_item("Settings",     "Ctrl+,")
    .add_item("Exit",         "Ctrl+Q")
    .cursor(&cursor)
    .highlight_style(style{colors::bright_white, colors::blue, attr::bold})
    .shadow();
con.print_widget(menu, 40);

// Modal popup
auto popup = popup_builder(text_builder("Are you sure?"))
    .title("Confirm")
    .dim_background(0.5f)
    .shadow();
```

### Live Display

```cpp
#include "tapiru/core/live.h"
#include "tapiru/widgets/progress.h"

console con;
auto task = std::make_shared<progress_task>("Downloading", 100);

{
    live lv(con, 12);  // 12 fps
    lv.set(progress_builder().add_task(task).bar_width(30));

    for (int i = 0; i <= 100; ++i) {
        task->advance(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}  // destructor stops render thread
```

### Spinner

```cpp
#include "tapiru/widgets/spinner.h"

auto sp = std::make_shared<spinner_state>();
{
    live lv(con);
    lv.set(spinner_builder(sp)
        .message("Loading...")
        .frames(spinner_frames::dots()));

    // ... do work ...
    sp->set_done();
}
```

### tqdm Progress Iterator

Header-only, no dependency on the tapiru shared library.

```cpp
#include "tapiru/core/tqdm.h"

// Wrap any iterable
std::vector<std::string> files = {"a.cpp", "b.cpp", "c.cpp"};
for (auto& f : tapiru::tqdm(files, "Compiling")) {
    compile(f);
}
// Output: Compiling  67%|████████████████████░░░░░░░░░░| 2/3 [00:01<00:00, 1.5files/s]

// Integer range (like Python tqdm(range(n)))
for (int i : tapiru::trange(1000).desc("Processing").unit("items").bar_width(35)) {
    process(i);
}

// Chained configuration
for (auto& x : tapiru::tqdm(data)
        .desc("Training")
        .bar_width(40)
        .unit("batch")
        .miniters(10)
        .colour(tapiru::color::from_rgb(0, 200, 100))) {
    train(x);
}
```

### Interactive Widgets

```cpp
#include "tapiru/widgets/interactive.h"

bool dark_mode = true;
int  theme_idx = 0;
float volume = 0.7f;
std::string name = "User";

rows_builder form;
form.add(checkbox_builder("Dark Mode", &dark_mode));
form.add(radio_group_builder({"Light", "Dark", "Monokai"}, &theme_idx));
form.add(slider_builder(&volume, 0.0f, 1.0f).width(30));
form.add(text_input_builder(&name).placeholder("Enter name").width(25));
con.print_widget(form, 60);
```

### Charts

```cpp
#include "tapiru/widgets/chart.h"

// Braille line chart
std::vector<float> data = {10, 25, 18, 42, 35, 50, 28, 60};
con.print_widget(line_chart_builder(data, 40, 6)
    .style_override(style{colors::bright_green}), 80);

// Block bar chart
con.print_widget(bar_chart_builder(data, 8)
    .labels({"Mon","Tue","Wed","Thu","Fri","Sat","Sun","Avg"})
    .style_override(style{colors::bright_cyan}), 80);
```

### Visual Effects

```cpp
#include "tapiru/core/shader.h"
#include "tapiru/core/gradient.h"

// Shadow and glow
auto p = panel_builder(text_builder("Content"))
    .shadow(2, 1, 1, color::from_rgb(0,0,0), 128)
    .glow(colors::cyan, 2, 100);

// Gradient border
auto tb = table_builder()
    .border_gradient({colors::cyan, colors::magenta, gradient_direction::horizontal});

// Shader effects
auto panel = panel_builder(text_builder("CRT Mode"))
    .shader(shaders::scanline(0.3f));
// Also: shaders::shimmer(), shaders::vignette(), shaders::glow_pulse()
```

### Logging

```cpp
#include "tapiru/core/logging.h"

console con;
log_handler logger(con);
logger.set_level(log_level::debug);

logger.info("Server started on port {}", 8080);
logger.warn("Connection pool at {}% capacity", 85);
logger.error("Failed to connect: {}", err_msg);
```

### Themes

```cpp
#include "tapiru/core/theme.h"

auto th = theme::dark();
th.define("danger", style{color::from_rgb(255,60,60), {}, attr::bold});
th.define("success", style{colors::bright_green});
// Use in markup: con.print("[.danger]Critical error![/]");
```

## Markup Reference

| Syntax                | Effect                            |
| --------------------- | --------------------------------- |
| `[bold]`              | Bold text                         |
| `[dim]`               | Dimmed text                       |
| `[italic]`            | Italic                            |
| `[underline]`         | Underline                         |
| `[strike]`            | Strikethrough                     |
| `[blink]`             | Blinking                          |
| `[reverse]`           | Reverse video                     |
| `[red]`               | Named foreground (16 ANSI colors) |
| `[on_blue]`           | Named background                  |
| `[bright_cyan]`       | Bright variant                    |
| `[#ff8800]`           | RGB foreground                    |
| `[on_#003366]`        | RGB background                    |
| `[bold red on_white]` | Combined attributes               |
| `[/]`                 | Reset to default                  |

Named colors: `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, and `bright_*` variants.

## Examples

| Example                 | Description                                          |
| ----------------------- | ---------------------------------------------------- |
| `demo.cpp`              | Comprehensive widget showcase                        |
| `tui_demo.cpp`          | Full TUI application demo                            |
| `vfx_demo.cpp`          | Visual effects and shaders                           |
| `new_features_demo.cpp` | Latest feature demonstrations                        |
| `interactive_demo.cpp`  | Keyboard/mouse interactive TUI with all widget types |

Build and run:
```bash
cmake --build build --config Release
./build/bin/Release/tapiru_demo
```

## Test Suite

27 test files covering all modules:

```bash
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## License

See LICENSE file for details.
