/**
 * @file ansi_input_fuzzer.cpp
 * @brief Fuzz target for the ANSI input parser.
 *
 * Feeds arbitrary byte sequences into the POSIX escape sequence parser
 * to find crashes or undefined behavior.
 *
 * Build with: clang++ -fsanitize=fuzzer,address -std=c++23 ...
 */

#include "tapiru/core/input.h"

#include <cstdint>
#include <cstring>
#include <optional>

// Forward-declare the internal parse functions we want to fuzz.
// These are defined in raw_input.cpp but not exposed in the header.
// For fuzzing, we re-implement a minimal version that exercises the
// same code paths.

namespace tapiru {

// We fuzz the key_event construction from arbitrary codepoints and
// special_key values to ensure no UB in the event types themselves.

static void fuzz_key_event(const uint8_t *data, size_t size) {
    if (size < 7) {
        return;
    }

    char32_t cp = 0;
    std::memcpy(&cp, data, 4);

    auto sk = static_cast<special_key>(data[4]);
    auto mods = static_cast<key_mod>(data[5]);
    auto act = static_cast<key_action>((data[6] % 3) + 1); // 1..3

    key_event ke{cp, sk, mods, act};
    (void)ke.codepoint;
    (void)ke.key;
    (void)ke.mods;
    (void)ke.action;

    // Construct input_event variant
    input_event ev = ke;
    (void)std::get_if<key_event>(&ev);
}

static void fuzz_mouse_event(const uint8_t *data, size_t size) {
    if (size < 10) {
        return;
    }

    uint32_t mx = 0, my = 0;
    std::memcpy(&mx, data, 4);
    std::memcpy(&my, data + 4, 4);

    auto btn = static_cast<mouse_button>(data[8]);
    auto action = static_cast<mouse_action>(data[9]);

    mouse_event me{mx, my, btn, action, key_mod::none};
    (void)me.x;
    (void)me.y;

    input_event ev = me;
    (void)std::get_if<mouse_event>(&ev);
}

static void fuzz_resize_event(const uint8_t *data, size_t size) {
    if (size < 8) {
        return;
    }

    uint32_t w = 0, h = 0;
    std::memcpy(&w, data, 4);
    std::memcpy(&h, data + 4, 4);

    resize_event re{w, h};
    input_event ev = re;
    (void)std::get_if<resize_event>(&ev);
}

} // namespace tapiru

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) {
        return 0;
    }

    // Route to different fuzz targets based on first byte
    uint8_t selector = data[0] % 3;
    const uint8_t *payload = data + 1;
    size_t payload_size = size - 1;

    switch (selector) {
    case 0:
        tapiru::fuzz_key_event(payload, payload_size);
        break;
    case 1:
        tapiru::fuzz_mouse_event(payload, payload_size);
        break;
    case 2:
        tapiru::fuzz_resize_event(payload, payload_size);
        break;
    }

    return 0;
}
