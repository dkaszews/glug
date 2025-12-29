// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/glob.hpp"

#include "parametrized.hpp"

#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace glug::glob::unit_test {

struct decompose_param {
    std::string glob{};
    decomposition expected{};
    decompose_mode mode{};
};

class decompose_test : public testing::TestWithParam<decompose_param> {};

// NOLINTNEXTLINE
TEST_P(decompose_test, pattern) {
    const auto& [glob, expected, mode] = GetParam();
    EXPECT_EQ(decompose(glob, mode).pattern, expected.pattern);
    EXPECT_EQ(decompose(glob + " ", mode).pattern, expected.pattern);
}

// NOLINTNEXTLINE
TEST_P(decompose_test, is_negative) {
    const auto& [glob, expected, mode] = GetParam();
    EXPECT_EQ(decompose(glob, mode).is_negative, expected.is_negative);
    EXPECT_EQ(decompose(glob + " ", mode).is_negative, expected.is_negative);
}

// NOLINTNEXTLINE
TEST_P(decompose_test, is_anchored) {
    const auto& [glob, expected, mode] = GetParam();
    EXPECT_EQ(decompose(glob, mode).is_anchored, expected.is_anchored);
    EXPECT_EQ(decompose(glob + " ", mode).is_anchored, expected.is_anchored);
}

// NOLINTNEXTLINE
TEST_P(decompose_test, is_directory) {
    const auto& [glob, expected, mode] = GetParam();
    EXPECT_EQ(decompose(glob, mode).is_directory, expected.is_directory);
    EXPECT_EQ(decompose(glob + " ", mode).is_directory, expected.is_directory);
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        empty,
        decompose_test,
        values_in<decompose_param>({
            { "", { "", false, false, false } },
            { "#", { "", false, false, false } },
            { "#a", { "", false, false, false } },
            { "#/", { "", false, false, false } },
            { "#a/b", { "", false, false, false } },
            { "#a/b", { "", false, false, false } },
            { "#!a/b", { "", false, false, false } },
            { "/", { "", false, false, false } },
            { "//", { "", false, false, false } },
            { "///", { "", false, false, false } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        simple,
        decompose_test,
        values_in<decompose_param>({
            { "a", { "a", false, false, false } },
            { "abc", { "abc", false, false, false } },
            { "-abc", { "-abc", false, false, false } },
            { "\\#abc", { "#abc", false, false, false } },
            { "\\##abc", { "##abc", false, false, false } },
            { "\\!abc", { "!abc", false, false, false } },
            { "\\!!abc", { "!!abc", false, false, false } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        trailing_whitespace,
        decompose_test,
        values_in<decompose_param>({
            { " ", { "", false, false, false } },
            { "a ", { "a", false, false, false } },
            { "a  ", { "a", false, false, false } },
            { "a\\ ", { "a\\ ", false, false, false } },
            { "a \\ ", { "a \\ ", false, false, false } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative,
        decompose_test,
        values_in<decompose_param>({
            { "!a", { "a", true, false, false } },
            { "!!a", { "!a", true, false, false } },
            { "!#a", { "#a", true, false, false } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        anchored,
        decompose_test,
        values_in<decompose_param>({
            { "/abc", { "abc", false, true, false } },
            { "//abc", { "abc", false, true, false } },
            { "///abc", { "abc", false, true, false } },
            { "a/bc", { "a/bc", false, true, false } },
            { "/a/bc", { "a/bc", false, true, false } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative_anchored,
        decompose_test,
        values_in<decompose_param>({
            { "!/abc", { "abc", true, true, false } },
            { "!a/bc", { "a/bc", true, true, false } },
            { "!/a/bc", { "a/bc", true, true, false } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        directory,
        decompose_test,
        values_in<decompose_param>({
            { "a/", { "a", false, false, true } },
            { "abc/", { "abc", false, false, true } },
            { "\\#a/", { "#a", false, false, true } },
            { "\\!a/", { "!a", false, false, true } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative_directory,
        decompose_test,
        values_in<decompose_param>({
            { "!a/", { "a", true, false, true } },
            { "!!a/", { "!a", true, false, true } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        anchored_directory,
        decompose_test,
        values_in<decompose_param>({
            { "/a/", { "a", false, true, true } },
            { "/abc/", { "abc", false, true, true } },
            { "/!a/b/c/", { "!a/b/c", false, true, true } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        negative_anchored_directory,
        decompose_test,
        values_in<decompose_param>({
            { "!/a/", { "a", true, true, true } },
            { "!/abc/", { "abc", true, true, true } },
            { "!/a/b/c/", { "a/b/c", true, true, true } },
        })
);

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        select_mode,
        decompose_test,
        values_in<decompose_param>({
            { "abc", { "abc", false, false, false }, decompose_mode::select },
            { "#abc", { "#abc", false, false, false }, decompose_mode::select },
            { "!abc", { "!abc", false, false, false }, decompose_mode::select },
            { "-abc", { "abc", true, false, false }, decompose_mode::select },
            { "/abc", { "abc", false, true, false }, decompose_mode::select },
            { "abc/", { "abc", false, false, true }, decompose_mode::select },
            { "-/abc", { "abc", true, true, false }, decompose_mode::select },
            { "-abc/", { "abc", true, false, true }, decompose_mode::select },
            { "-/abc/", { "abc", true, true, true }, decompose_mode::select },
        })
);

struct split_param {
    std::string input{};
    std::vector<std::string_view> expected{};
    char delimiter{ ',' };
};

class split_test : public testing::TestWithParam<split_param> {};

// NOLINTNEXTLINE
TEST_P(split_test, test) {
    const auto actual = split(GetParam().input, GetParam().delimiter);
    EXPECT_EQ(actual, GetParam().expected);
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        split_test,
        split_test,
        values_in<split_param>({
            { "", {} },
            { "a", { "a" } },
            { "abc", { "abc" } },
            { "abc,def", { "abc", "def" } },
            { "abc,def,xyz", { "abc", "def", "xyz" } },
            { "abc,", { "abc" } },
            { ",abc", { "abc" } },
            { ",abc,,xyz,", { "abc", "xyz" } },
            { "\\abc", { "\\abc" } },
            { "abc\\", { "abc\\" } },
            { "\\abc\\", { "\\abc\\" } },
            { "abc\\,xyz", { "abc\\,xyz" } },
            { "abc\\\\,xyz", { "abc\\\\", "xyz" } },
            { "abc\\\\\\,xyz", { "abc\\\\\\,xyz" } },
            { "abc\\ ,xyz", { "abc\\ ", "xyz" } },
            { "abc\\\\ ,xyz", { "abc\\\\ ", "xyz" } },
            { "abc\\\\\\ ,xyz", { "abc\\\\\\ ", "xyz" } },
            { "abc\\ \\,xyz", { "abc\\ \\,xyz" } },
            { "abc\\ \\\\,xyz", { "abc\\ \\\\", "xyz" } },
            { "abc,def", { "abc,def" }, ':' },
            { "abc:def", { "abc", "def" }, ':' },
            { "abc,def:xyz", { "abc,def", "xyz" }, ':' },
            { "abc\\:xyz", { "abc\\:xyz" }, ':' },
            { "abc\\\\:xyz", { "abc\\\\", "xyz" }, ':' },
        })
);

}  // namespace glug::glob::unit_test

