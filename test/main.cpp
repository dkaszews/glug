#include "gtest/gtest.h"

// Windows does not like linking against packaged gtest main
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

