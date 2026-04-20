#include <gtest/gtest.h>

#include "tapiru/tapiru.h"

/**
 * @brief Verify that the tapiru shared library links and the umbrella header compiles.
 */
TEST(ScaffoldingTest, UmbrellaHeaderCompiles) {
    EXPECT_TRUE(true);
}

TEST(ScaffoldingTest, VersionReturnsNonNull) {
    const char* v = tapiru::version();
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "0.1.0");
}
