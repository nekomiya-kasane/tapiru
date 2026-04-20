add_executable(tapiru_demo demo.cpp)
target_link_libraries(tapiru_demo PRIVATE tapiru)

add_executable(tapiru_tui_demo tui_demo.cpp)
target_link_libraries(tapiru_tui_demo PRIVATE tapiru)

add_executable(tapiru_vfx_demo vfx_demo.cpp)
target_link_libraries(tapiru_vfx_demo PRIVATE tapiru)

add_executable(tapiru_new_features_demo new_features_demo.cpp)
target_link_libraries(tapiru_new_features_demo PRIVATE tapiru)

add_executable(tapiru_interactive_demo interactive_demo.cpp)
target_link_libraries(tapiru_interactive_demo PRIVATE tapiru)

add_executable(tapiru_widgets_demo widgets_demo.cpp)
target_link_libraries(tapiru_widgets_demo PRIVATE tapiru)

add_executable(tapiru_interactive_widgets_demo interactive_widgets_demo.cpp)
target_link_libraries(tapiru_interactive_widgets_demo PRIVATE tapiru)

add_executable(tapiru_classic_app_demo classic_app_demo.cpp)
target_link_libraries(tapiru_classic_app_demo PRIVATE tapiru)

add_executable(tapiru_stage5_7_demo stage5_7_demo.cpp)
target_link_libraries(tapiru_stage5_7_demo PRIVATE tapiru)

add_executable(tapiru_comprehensive_demo comprehensive_demo.cpp)
target_link_libraries(tapiru_comprehensive_demo PRIVATE tapiru)

add_executable(tapiru_vim_demo vim_demo.cpp)
target_link_libraries(tapiru_vim_demo PRIVATE tapiru)

add_executable(tapiru_markup_demo markup_demo.cpp)
target_link_libraries(tapiru_markup_demo PRIVATE tapiru)

add_executable(tapiru_showcase_demo showcase_demo.cpp)
target_link_libraries(tapiru_showcase_demo PRIVATE tapiru)

add_executable(tapiru_highlighter_demo highlighter_demo.cpp)
target_link_libraries(tapiru_highlighter_demo PRIVATE tapiru)

add_executable(tapiru_dashboard tapiru_dashboard.cpp)
target_link_libraries(tapiru_dashboard PRIVATE tapiru)

add_executable(stout_dashboard stout_dashboard.cpp)
target_link_libraries(stout_dashboard PRIVATE tapiru)

add_executable(frappe_dashboard frappe_dashboard.cpp)
target_link_libraries(frappe_dashboard PRIVATE tapiru)

add_executable(gin_dashboard gin_dashboard.cpp)
target_link_libraries(gin_dashboard PRIVATE tapiru)

add_executable(spearmint_dashboard spearmint_dashboard.cpp)
target_link_libraries(spearmint_dashboard PRIVATE tapiru)

if(MSVC)
    target_compile_options(tapiru_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_tui_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_tui_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_vfx_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_vfx_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_new_features_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_new_features_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_interactive_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_interactive_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_widgets_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_widgets_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_interactive_widgets_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_interactive_widgets_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_classic_app_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_classic_app_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_stage5_7_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_stage5_7_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_comprehensive_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_comprehensive_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_vim_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_vim_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_markup_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_markup_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_showcase_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_showcase_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_highlighter_demo PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_highlighter_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapiru_dashboard PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(tapiru_dashboard PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(stout_dashboard PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(stout_dashboard PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(frappe_dashboard PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(frappe_dashboard PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(gin_dashboard PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(gin_dashboard PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(spearmint_dashboard PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(spearmint_dashboard PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
