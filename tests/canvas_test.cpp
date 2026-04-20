#include <gtest/gtest.h>

#include "tapiru/core/canvas.h"

using namespace tapiru;

// ── Cell tests ──────────────────────────────────────────────────────────

TEST(CellTest, DefaultCell) {
    cell c;
    EXPECT_EQ(c.codepoint, U' ');
    EXPECT_EQ(c.sid, default_style_id);
    EXPECT_EQ(c.width, 1);
}

TEST(CellTest, SizeIs8Bytes) {
    static_assert(sizeof(cell) == 8);
}

TEST(CellTest, Equality) {
    cell a{U'A', 1, 1, 255};
    cell b{U'A', 1, 1, 255};
    cell c{U'B', 1, 1, 255};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(CellTest, DifferentStyleNotEqual) {
    cell a{U'A', 0, 1, 255};
    cell b{U'A', 1, 1, 255};
    EXPECT_NE(a, b);
}

// ── Canvas construction ─────────────────────────────────────────────────

TEST(CanvasTest, DefaultConstruction) {
    canvas cv;
    EXPECT_EQ(cv.width(), 0u);
    EXPECT_EQ(cv.height(), 0u);
    EXPECT_EQ(cv.cell_count(), 0u);
}

TEST(CanvasTest, SizedConstruction) {
    canvas cv(80, 24);
    EXPECT_EQ(cv.width(), 80u);
    EXPECT_EQ(cv.height(), 24u);
    EXPECT_EQ(cv.cell_count(), 80u * 24u);
}

TEST(CanvasTest, DefaultCellsAreSpaces) {
    canvas cv(3, 2);
    for (uint32_t y = 0; y < 2; ++y) {
        for (uint32_t x = 0; x < 3; ++x) {
            auto& c = cv.get(x, y);
            EXPECT_EQ(c.codepoint, U' ');
            EXPECT_EQ(c.sid, default_style_id);
        }
    }
}

// ── Set / Get ───────────────────────────────────────────────────────────

TEST(CanvasTest, SetAndGet) {
    canvas cv(10, 5);
    cell c{U'X', 2, 1, 255};
    cv.set(3, 4, c);
    EXPECT_EQ(cv.get(3, 4), c);
}

TEST(CanvasTest, SetDoesNotAffectCurrent) {
    canvas cv(10, 5);
    cell c{U'X', 2, 1, 255};
    cv.set(3, 4, c);
    // current buffer should still be default
    EXPECT_EQ(cv.get_current(3, 4).codepoint, U' ');
}

// ── Diff ────────────────────────────────────────────────────────────────

TEST(CanvasTest, NoDiffOnFreshCanvas) {
    canvas cv(10, 5);
    EXPECT_EQ(cv.diff_count(), 0u);
}

TEST(CanvasTest, DiffDetectsChanges) {
    canvas cv(10, 5);
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    cv.set(9, 4, cell{U'Z', 0, 1, 255});
    EXPECT_EQ(cv.diff_count(), 2u);
}

TEST(CanvasTest, DiffCallbackReportsCorrectCells) {
    canvas cv(5, 3);
    cell target{U'!', 1, 1, 255};
    cv.set(2, 1, target);

    size_t count = 0;
    cv.diff([&](uint32_t x, uint32_t y, const cell& c) {
        EXPECT_EQ(x, 2u);
        EXPECT_EQ(y, 1u);
        EXPECT_EQ(c, target);
        ++count;
    });
    EXPECT_EQ(count, 1u);
}

TEST(CanvasTest, DiffAfterSwapReflectsPointerSwap) {
    canvas cv(10, 5);
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    EXPECT_EQ(cv.diff_count(), 1u);
    cv.swap();
    // After O(1) swap: current has 'A' at (0,0), next is old current (all spaces).
    // So next[0,0]=space != current[0,0]='A' → diff_count = 1
    EXPECT_EQ(cv.diff_count(), 1u);
    // To get zero diff, write the same content into next:
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    EXPECT_EQ(cv.diff_count(), 0u);
}

// ── Swap ────────────────────────────────────────────────────────────────

TEST(CanvasTest, SwapPromotesNextToCurrent) {
    canvas cv(5, 5);
    cell c{U'Q', 3, 2, 255};
    cv.set(1, 1, c);
    cv.swap();
    EXPECT_EQ(cv.get_current(1, 1), c);
}

TEST(CanvasTest, SwapIsO1PointerSwap) {
    // After swap, the old current (all spaces) becomes next,
    // and the old next (with 'Q') becomes current.
    canvas cv(5, 5);
    cell c{U'Q', 3, 2, 255};
    cv.set(1, 1, c);
    cv.swap();
    // current now has 'Q'
    EXPECT_EQ(cv.get_current(1, 1), c);
    // next is now the old current (all spaces) — diff should show 1 cell
    // because next[1,1] is space but current[1,1] is 'Q'
    EXPECT_EQ(cv.diff_count(), 1u);
}

// ── Invalidate current ─────────────────────────────────────────────────

TEST(CanvasTest, InvalidateCurrentForcesFullDiff) {
    canvas cv(3, 2);
    // Write some cells and swap so current matches next
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    cv.swap();
    // Now write the same cell to next — should be 0 diff after re-setting
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    EXPECT_EQ(cv.diff_count(), 0u);

    // Invalidate current — every cell should now diff
    cv.invalidate_current();
    EXPECT_EQ(cv.diff_count(), 6u);
}

// ── Clear ───────────────────────────────────────────────────────────────

TEST(CanvasTest, ClearNextResetsToDefault) {
    canvas cv(5, 5);
    cv.set(0, 0, cell{U'X', 1, 1, 255});
    cv.clear_next();
    EXPECT_EQ(cv.get(0, 0).codepoint, U' ');
    EXPECT_EQ(cv.diff_count(), 0u);
}

// ── Resize ──────────────────────────────────────────────────────────────

TEST(CanvasTest, ResizeClearsBothBuffers) {
    canvas cv(10, 10);
    cv.set(5, 5, cell{U'A', 0, 1, 255});
    cv.swap();
    cv.set(3, 3, cell{U'B', 0, 1, 255});

    cv.resize(20, 10);
    EXPECT_EQ(cv.width(), 20u);
    EXPECT_EQ(cv.height(), 10u);
    EXPECT_EQ(cv.cell_count(), 200u);
    EXPECT_EQ(cv.diff_count(), 0u);
}

// ── Buffer access ───────────────────────────────────────────────────────

TEST(CanvasTest, BufferAccessSize) {
    canvas cv(4, 3);
    EXPECT_EQ(cv.next_buffer().size(), 12u);
    EXPECT_EQ(cv.current_buffer().size(), 12u);
}

// ── Full-screen diff scenario ───────────────────────────────────────────

TEST(CanvasTest, FullScreenWrite) {
    canvas cv(3, 2);
    // Write every cell in next buffer
    for (uint32_t y = 0; y < 2; ++y) {
        for (uint32_t x = 0; x < 3; ++x) {
            cv.set(x, y, cell{static_cast<char32_t>(U'A' + y * 3 + x), 0, 1, 255});
        }
    }
    // All 6 cells should differ from the default current buffer
    EXPECT_EQ(cv.diff_count(), 6u);

    cv.swap();
    // After O(1) swap: current has A-F, next is old current (all spaces) → 6 diffs
    EXPECT_EQ(cv.diff_count(), 6u);

    // Verify current buffer contents
    EXPECT_EQ(cv.get_current(0, 0).codepoint, U'A');
    EXPECT_EQ(cv.get_current(2, 1).codepoint, U'F');
}

// ── Alpha compositing ──────────────────────────────────────────────────

TEST(CanvasTest, TransparentCellIsSkipped) {
    canvas cv(5, 5);
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    // Write transparent cell — should NOT overwrite
    cv.set(0, 0, cell{U'B', 0, 1, 0});
    EXPECT_EQ(cv.get(0, 0).codepoint, U'A');
}

TEST(CanvasTest, OpaqueCellOverwrites) {
    canvas cv(5, 5);
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    cv.set(0, 0, cell{U'B', 0, 1, 255});
    EXPECT_EQ(cv.get(0, 0).codepoint, U'B');
}

TEST(CanvasTest, DefaultCellAlphaIs255) {
    cell c;
    EXPECT_EQ(c.alpha, 255);
}

TEST(CanvasTest, SetBlendedFullyOpaque) {
    style_table st;
    canvas cv(5, 5);
    auto sid = st.intern(style{color::from_rgb(255, 0, 0), color::from_rgb(0, 0, 0)});
    cv.set_blended(0, 0, cell{U'X', sid, 1, 255}, st);
    EXPECT_EQ(cv.get(0, 0).codepoint, U'X');
}

TEST(CanvasTest, SetBlendedFullyTransparent) {
    style_table st;
    canvas cv(5, 5);
    cv.set(0, 0, cell{U'A', 0, 1, 255});
    auto sid = st.intern(style{color::from_rgb(255, 0, 0), color::from_rgb(0, 0, 0)});
    cv.set_blended(0, 0, cell{U'X', sid, 1, 0}, st);
    // Transparent — should not overwrite
    EXPECT_EQ(cv.get(0, 0).codepoint, U'A');
}

TEST(CanvasTest, SetBlendedHalfAlpha) {
    style_table st;
    canvas cv(5, 5);
    // Destination: white bg
    auto dst_sid = st.intern(style{color::from_rgb(255, 255, 255), color::from_rgb(255, 255, 255)});
    cv.set(0, 0, cell{U'A', dst_sid, 1, 255});
    // Source: black fg/bg at ~50% alpha (128)
    auto src_sid = st.intern(style{color::from_rgb(0, 0, 0), color::from_rgb(0, 0, 0)});
    cv.set_blended(0, 0, cell{U'B', src_sid, 1, 128}, st);
    // Result should be blended and fully opaque
    auto result = cv.get(0, 0);
    EXPECT_EQ(result.codepoint, U'B');
    EXPECT_EQ(result.alpha, 255);
    // Blended color: (0*128 + 255*127)/255 ≈ 127
    auto blended_sty = st.lookup(result.sid);
    EXPECT_GT(blended_sty.fg.r, 100);
    EXPECT_LT(blended_sty.fg.r, 160);
}
