#pragma once

/**
 * @file border_join.h
 * @brief Post-processing pass to merge adjacent border characters on the canvas.
 *
 * Scans the canvas for box-drawing characters and replaces corners/edges
 * with proper T-junctions or crosses when adjacent borders meet.
 * Only merges characters from the same border_style.
 */

#include "tapiru/core/canvas.h"
#include "tapiru/core/cell.h"
#include "tapiru/layout/types.h"

#include <cstdint>

namespace tapiru::detail {

using tapiru::cell;

/**
 * @brief Identify which border_style a box-drawing codepoint belongs to.
 * Returns border_style::none if the codepoint is not a recognized border char.
 */
inline border_style classify_border_char(char32_t cp) {
    // Rounded light box-drawing
    switch (cp) {
    case U'\x256D':
    case U'\x256E':
    case U'\x2570':
    case U'\x256F':
    case U'\x2500':
    case U'\x2502':
    case U'\x253C':
    case U'\x252C':
    case U'\x2534':
    case U'\x251C':
    case U'\x2524':
        return border_style::rounded;
    case U'\x250F':
    case U'\x2513':
    case U'\x2517':
    case U'\x251B':
    case U'\x2501':
    case U'\x2503':
    case U'\x254B':
    case U'\x2533':
    case U'\x253B':
    case U'\x2523':
    case U'\x252B':
        return border_style::heavy;
    case U'\x2554':
    case U'\x2557':
    case U'\x255A':
    case U'\x255D':
    case U'\x2550':
    case U'\x2551':
    case U'\x256C':
    case U'\x2566':
    case U'\x2569':
    case U'\x2560':
    case U'\x2563':
        return border_style::double_;
    case '+':
    case '-':
    case '|':
        return border_style::ascii;
    default:
        return border_style::none;
    }
}

/** @brief Check if a codepoint has a horizontal component (─, corners, T-junctions, cross). */
inline bool has_horizontal(char32_t cp, border_style bs) {
    auto bc = get_border_chars(bs);
    return cp == bc.horizontal || cp == bc.top_left || cp == bc.top_right || cp == bc.bottom_left ||
           cp == bc.bottom_right || cp == bc.cross || cp == bc.t_down || cp == bc.t_up || cp == bc.t_right ||
           cp == bc.t_left;
}

/** @brief Check if a codepoint has a vertical component (│, corners, T-junctions, cross). */
inline bool has_vertical(char32_t cp, border_style bs) {
    auto bc = get_border_chars(bs);
    return cp == bc.vertical || cp == bc.top_left || cp == bc.top_right || cp == bc.bottom_left ||
           cp == bc.bottom_right || cp == bc.cross || cp == bc.t_down || cp == bc.t_up || cp == bc.t_right ||
           cp == bc.t_left;
}

/**
 * @brief Determine the correct join character based on which directions connect.
 *
 * @param up    true if the cell above has a vertical component
 * @param down  true if the cell below has a vertical component
 * @param left  true if the cell to the left has a horizontal component
 * @param right true if the cell to the right has a horizontal component
 */
inline char32_t select_join(bool up, bool down, bool left, bool right, border_style bs) {
    auto bc = get_border_chars(bs);
    // up=1, down=2, left=4, right=8
    int mask = (up ? 1 : 0) | (down ? 2 : 0) | (left ? 4 : 0) | (right ? 8 : 0);
    switch (mask) {
    case 15:
        return bc.cross; // ┼  all four
    case 14:
        return bc.t_down; // ┬  down+left+right
    case 13:
        return bc.t_up; // ┴  up+left+right
    case 11:
        return bc.t_right; // ├  up+down+right
    case 7:
        return bc.t_left; // ┤  up+down+left
    case 10:
        return bc.top_left; // ╭  down+right
    case 6:
        return bc.top_right; // ╮  down+left
    case 9:
        return bc.bottom_left; // ╰  up+right
    case 5:
        return bc.bottom_right; // ╯  up+left
    case 12:
        return bc.horizontal; // ─  left+right
    case 3:
        return bc.vertical; // │  up+down
    default:
        return 0;
    }
}

/**
 * @brief Post-process the canvas to merge adjacent border characters.
 *
 * For each cell that is a border character, check its 4 neighbors.
 * If neighbors are border chars of the same style, replace the current
 * cell with the appropriate join character.
 */
inline void apply_border_joins(canvas &cv) {
    uint32_t w = cv.width();
    uint32_t h = cv.height();
    if (w == 0 || h == 0) return;

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            auto &c = cv.get(x, y);
            auto bs = classify_border_char(c.codepoint);
            if (bs == border_style::none) continue;

            // Check neighbors (same border_style only)
            bool up = (y > 0) && classify_border_char(cv.get(x, y - 1).codepoint) == bs &&
                      has_vertical(cv.get(x, y - 1).codepoint, bs);
            bool down = (y < h - 1) && classify_border_char(cv.get(x, y + 1).codepoint) == bs &&
                        has_vertical(cv.get(x, y + 1).codepoint, bs);
            bool left = (x > 0) && classify_border_char(cv.get(x - 1, y).codepoint) == bs &&
                        has_horizontal(cv.get(x - 1, y).codepoint, bs);
            bool right = (x < w - 1) && classify_border_char(cv.get(x + 1, y).codepoint) == bs &&
                         has_horizontal(cv.get(x + 1, y).codepoint, bs);

            char32_t join = select_join(up, down, left, right, bs);
            if (join != 0 && join != c.codepoint) {
                cv.set(x, y, cell{join, c.sid, c.width, c.alpha});
            }
        }
    }
}

} // namespace tapiru::detail
