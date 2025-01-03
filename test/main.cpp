#include <gtest/gtest.h>

TEST(foo, two_plus_two_equals_four) { EXPECT_EQ(2 + 2, 4); }

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

