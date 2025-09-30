// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "gtest/gtest.h"

// Windows does not like linking against packaged gtest main
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

