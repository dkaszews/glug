// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/glob.hpp"

#include "parametrized.hpp"

#include <string>
#include <tuple>

#include <gtest/gtest.h>

namespace glug::glob::unit_test {

using decompose_param = std::tuple<std::string, std::string, bool, bool, bool>;

class decompose_test : public testing::TestWithParam<decompose_param> {};

// NOLINTNEXTLINE
TEST_P(decompose_test, pattern) {
    const auto glob = std::get<0>(GetParam());
    const auto pattern = std::get<1>(GetParam());
    EXPECT_EQ(decompose(glob).pattern, pattern);
    EXPECT_EQ(decompose(glob + " ").pattern, pattern);
}

// NOLINTNEXTLINE
TEST_P(decompose_test, is_negative) {
    const auto glob = std::get<0>(GetParam());
    const auto is_negative = std::get<2>(GetParam());
    EXPECT_EQ(decompose(glob).is_negative, is_negative);
    EXPECT_EQ(decompose(glob + " ").is_negative, is_negative);
}

// NOLINTNEXTLINE
TEST_P(decompose_test, is_anchored) {
    const auto glob = std::get<0>(GetParam());
    const auto is_anchored = std::get<3>(GetParam());
    EXPECT_EQ(decompose(glob).is_anchored, is_anchored);
    EXPECT_EQ(decompose(glob + " ").is_anchored, is_anchored);
}

// NOLINTNEXTLINE
TEST_P(decompose_test, is_directory) {
    const auto glob = std::get<0>(GetParam());
    const auto is_directory = std::get<4>(GetParam());
    EXPECT_EQ(decompose(glob).is_directory, is_directory);
    EXPECT_EQ(decompose(glob + " ").is_directory, is_directory);
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        empty,
        decompose_test,
        values_in<decompose_param>({
            { "", "", false, false, false },
            { "#", "", false, false, false },
            { "#a", "", false, false, false },
            { "#/", "", false, false, false },
            { "#a/b", "", false, false, false },
            { "#a/b", "", false, false, false },
            { "#!a/b", "", false, false, false },
            { "/", "", false, false, false },
            { "//", "", false, false, false },
            { "///", "", false, false, false },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        simple,
        decompose_test,
        values_in<decompose_param>({
            { "a", "a", false, false, false },
            { "abc", "abc", false, false, false },
            { "\\#abc", "#abc", false, false, false },
            { "\\##abc", "##abc", false, false, false },
            { "\\!abc", "!abc", false, false, false },
            { "\\!!abc", "!!abc", false, false, false },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        trailing_whitespace,
        decompose_test,
        values_in<decompose_param>({
            { " ", "", false, false, false },
            { "a ", "a", false, false, false },
            { "a  ", "a", false, false, false },
            { "a\\ ", "a\\ ", false, false, false },
            { "a \\ ", "a \\ ", false, false, false },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative,
        decompose_test,
        values_in<decompose_param>({
            { "!a", "a", true, false, false },
            { "!!a", "!a", true, false, false },
            { "!#a", "#a", true, false, false },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        anchored,
        decompose_test,
        values_in<decompose_param>({
            { "/abc", "abc", false, true, false },
            { "//abc", "abc", false, true, false },
            { "///abc", "abc", false, true, false },
            { "a/bc", "a/bc", false, true, false },
            { "/a/bc", "a/bc", false, true, false },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative_anchored,
        decompose_test,
        values_in<decompose_param>({
            { "!/abc", "abc", true, true, false },
            { "!a/bc", "a/bc", true, true, false },
            { "!/a/bc", "a/bc", true, true, false },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        directory,
        decompose_test,
        values_in<decompose_param>({
            { "a/", "a", false, false, true },
            { "abc/", "abc", false, false, true },
            { "\\#a/", "#a", false, false, true },
            { "\\!a/", "!a", false, false, true },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative_directory,
        decompose_test,
        values_in<decompose_param>({
            { "!a/", "a", true, false, true },
            { "!!a/", "!a", true, false, true },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        anchored_directory,
        decompose_test,
        values_in<decompose_param>({
            { "/a/", "a", false, true, true },
            { "/abc/", "abc", false, true, true },
            { "/!a/b/c/", "!a/b/c", false, true, true },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative_anchored_directory,
        decompose_test,
        values_in<decompose_param>({
            { "!/a/", "a", true, true, true },
            { "!/abc/", "abc", true, true, true },
            { "!/a/b/c/", "a/b/c", true, true, true },
        })
);

}  // namespace glug::glob::unit_test

