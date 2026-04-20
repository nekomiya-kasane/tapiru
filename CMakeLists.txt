# tapiru — C++23 Terminal & TUI library (COCA workspace build)
# Provides: tapiru (SHARED), tapiru::tapiru

# ── Sources ──────────────────────────────────────────────────────────────
set(TAPIRU_SOURCES
    src/tapiru.cpp
    src/core/console.cpp
    src/text/markup.cpp
    src/detail/widgets.cpp
    src/widgets/builders.cpp
    src/widgets/progress.cpp
    src/widgets/spinner.cpp
    src/widgets/interactive.cpp
    src/widgets/virtual_list.cpp
    src/widgets/chart.cpp
    src/widgets/image.cpp
    src/widgets/status_bar.cpp
    src/widgets/menu.cpp
    src/widgets/scroll_view.cpp
    src/widgets/menu_bar.cpp
    src/widgets/tab.cpp
    src/widgets/accordion.cpp
    src/widgets/tree_view.cpp
    src/widgets/dropdown.cpp
    src/widgets/toggle.cpp
    src/widgets/toast.cpp
    src/widgets/textarea.cpp
    src/widgets/log_viewer.cpp
    src/widgets/search_input.cpp
    src/widgets/breadcrumb.cpp
    src/widgets/keybinding.cpp
    src/widgets/grid.cpp
    src/widgets/popup.cpp
    src/widgets/form.cpp
    src/widgets/classic_app.cpp
    src/widgets/component_factories.cpp
    src/widgets/app_components.cpp
    src/widgets/canvas_widget.cpp
    src/widgets/gauge.cpp
    src/widgets/paragraph.cpp
    src/core/theme.cpp
    src/core/input.cpp
    src/core/element.cpp
    src/core/decorator.cpp
    src/core/component.cpp
    src/core/screen.cpp
    src/core/live.cpp
    src/core/logging.cpp
    src/text/emoji.cpp
    src/text/markdown.cpp
    src/text/highlighter.cpp
    src/core/accessibility.cpp
)

# ── Shared library ───────────────────────────────────────────────────────
if(EMSCRIPTEN)
    add_library(tapiru STATIC ${TAPIRU_SOURCES})
else()
    add_library(tapiru SHARED ${TAPIRU_SOURCES})
endif()
add_library(tapiru::tapiru ALIAS tapiru)

target_include_directories(tapiru
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(tapiru PRIVATE TAPIRU_BUILD_INTERNAL)

target_link_libraries(tapiru PUBLIC tapioca::tapioca)

if(MSVC)
    target_compile_definitions(tapiru PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru PRIVATE /W4 /utf-8 /permissive-)
else()
    target_compile_options(tapiru PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Tests, examples, fuzz ─────────────────────────────────────────────
if(COCA_ENABLE_TESTS AND TARGET gtest)
    add_subdirectory(tests)
endif()
add_subdirectory(examples)
if(TAPIRU_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()
