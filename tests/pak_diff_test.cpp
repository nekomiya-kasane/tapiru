/**
 * @file pak_diff_test.cpp
 * @brief Tests for incremental .lpak diff/patch types and header format.
 */

#include "smoothie/resource/pak_diff.h"

#include <gtest/gtest.h>

using namespace smoothie::resource;

// ── patch_op enum ───────────────────────────────────────────────────────

TEST(PakDiffTest, PatchOpValues) {
    EXPECT_EQ(static_cast<uint8_t>(patch_op::keep), 0u);
    EXPECT_EQ(static_cast<uint8_t>(patch_op::add), 1u);
    EXPECT_EQ(static_cast<uint8_t>(patch_op::modify), 2u);
    EXPECT_EQ(static_cast<uint8_t>(patch_op::remove), 3u);
}

// ── patch_entry defaults ────────────────────────────────────────────────

TEST(PakDiffTest, PatchEntryDefaults) {
    patch_entry e;
    EXPECT_EQ(e.op, patch_op::keep);
    EXPECT_EQ(e.semantic_hash, 0u);
    EXPECT_EQ(e.type, 0u);
    EXPECT_EQ(e.flags, 0u);
    EXPECT_EQ(e.new_data_size, 0u);
    EXPECT_EQ(e.old_data_size, 0u);
    EXPECT_EQ(e.patch_offset, 0u);
}

TEST(PakDiffTest, PatchEntryAddOp) {
    patch_entry e;
    e.op = patch_op::add;
    e.semantic_hash = 0xDEADBEEF;
    e.new_data_size = 1024;
    EXPECT_EQ(e.op, patch_op::add);
    EXPECT_EQ(e.semantic_hash, 0xDEADBEEF);
    EXPECT_EQ(e.new_data_size, 1024u);
    EXPECT_EQ(e.old_data_size, 0u); // new entry has no old data
}

TEST(PakDiffTest, PatchEntryModifyOp) {
    patch_entry e;
    e.op = patch_op::modify;
    e.semantic_hash = 0xCAFEBABE;
    e.old_data_size = 512;
    e.new_data_size = 768;
    e.patch_offset = 4096;
    EXPECT_EQ(e.op, patch_op::modify);
    EXPECT_EQ(e.old_data_size, 512u);
    EXPECT_EQ(e.new_data_size, 768u);
    EXPECT_EQ(e.patch_offset, 4096u);
}

TEST(PakDiffTest, PatchEntryRemoveOp) {
    patch_entry e;
    e.op = patch_op::remove;
    e.semantic_hash = 0x12345678;
    e.old_data_size = 256;
    EXPECT_EQ(e.op, patch_op::remove);
    EXPECT_EQ(e.new_data_size, 0u); // remove has no new data
}

// ── patch_header format ─────────────────────────────────────────────────

TEST(PakDiffTest, LpatchMagic) {
    EXPECT_EQ(lpatch_magic[0], 'L');
    EXPECT_EQ(lpatch_magic[1], 'P');
    EXPECT_EQ(lpatch_magic[2], 'C');
    EXPECT_EQ(lpatch_magic[3], 'H');
}

TEST(PakDiffTest, LpatchVersion) {
    EXPECT_EQ(lpatch_version, 1u);
}

TEST(PakDiffTest, PatchHeaderLayout) {
    patch_header hdr{};
    hdr.magic[0] = 'L';
    hdr.magic[1] = 'P';
    hdr.magic[2] = 'C';
    hdr.magic[3] = 'H';
    hdr.version = lpatch_version;
    hdr.entry_count = 10;
    hdr.old_data_checksum = 0xAAAABBBBCCCCDDDD;
    hdr.new_data_checksum = 0x1111222233334444;
    hdr.data_payload_size = 65536;
    hdr.added_count = 3;
    hdr.modified_count = 4;
    hdr.removed_count = 1;
    hdr.kept_count = 2;

    EXPECT_EQ(hdr.magic[0], 'L');
    EXPECT_EQ(hdr.version, 1u);
    EXPECT_EQ(hdr.entry_count, 10u);
    EXPECT_EQ(hdr.added_count + hdr.modified_count + hdr.removed_count + hdr.kept_count, 10u);
    EXPECT_EQ(hdr.data_payload_size, 65536u);
}

TEST(PakDiffTest, PatchHeaderPacked) {
    // patch_header is pack(1), verify it doesn't have unexpected padding
    // magic(4) + version(4) + entry_count(4) + old_cksum(8) + new_cksum(8)
    // + payload_size(8) + added(4) + modified(4) + removed(4) + kept(4) = 52
    EXPECT_EQ(sizeof(patch_header), 52u);
}

// ── Validate patch stats consistency ────────────────────────────────────

TEST(PakDiffTest, StatsConsistency) {
    // Simulated scenario: 100 entries in old, 110 in new
    // 80 kept, 10 modified, 20 added, 10 removed
    // old_entries = kept + modified + removed = 80+10+10 = 100
    // new_entries = kept + modified + added   = 80+10+20 = 110
    uint32_t kept = 80, modified = 10, added = 20, removed = 10;
    EXPECT_EQ(kept + modified + removed, 100u);
    EXPECT_EQ(kept + modified + added, 110u);
}
