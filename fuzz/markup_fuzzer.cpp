/**
 * @file markup_fuzzer.cpp
 * @brief Fuzz target for the markup parser.
 *
 * Build with: clang++ -fsanitize=fuzzer,address -std=c++23 ...
 */

#include "tapiru/text/markup.h"

#include <cstdint>
#include <string>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    std::string_view input(reinterpret_cast<const char *>(data), size);

    // Fuzz parse_markup — should never crash regardless of input
    auto fragments = tapiru::parse_markup(std::string(input));
    (void)fragments;

    // Fuzz strip_markup
    auto stripped = tapiru::strip_markup(std::string(input));
    (void)stripped;

    return 0;
}
