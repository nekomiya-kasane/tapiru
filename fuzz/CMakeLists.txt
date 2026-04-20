# Fuzz targets — requires Clang with -fsanitize=fuzzer support.
# Build: cmake -B build-fuzz -DTAPIRU_BUILD_FUZZERS=ON -DCMAKE_CXX_COMPILER=clang++ ..
#        cmake --build build-fuzz

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "Fuzz targets require Clang. Skipping.")
    return()
endif()

set(FUZZ_FLAGS "-fsanitize=fuzzer,address -fno-omit-frame-pointer")

# markup_fuzzer
add_executable(markup_fuzzer markup_fuzzer.cpp)
target_link_libraries(markup_fuzzer PRIVATE tapiru)
target_compile_options(markup_fuzzer PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
target_link_options(markup_fuzzer PRIVATE -fsanitize=fuzzer,address)

# ansi_input_fuzzer
add_executable(ansi_input_fuzzer ansi_input_fuzzer.cpp)
target_link_libraries(ansi_input_fuzzer PRIVATE tapiru)
target_compile_options(ansi_input_fuzzer PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
target_link_options(ansi_input_fuzzer PRIVATE -fsanitize=fuzzer,address)
