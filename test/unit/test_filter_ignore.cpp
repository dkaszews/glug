// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filter.hpp"

#include "tree.hpp"

#include <filesystem>
#include <optional>
#include <ostream>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glug::filter::unit_test {

using glug::unit_test::operator""_d;
using glug::unit_test::operator""_f;

struct ignore_param {
    std::vector<std::string_view> globs{};
    std::vector<std::pair<glug::unit_test::node, decision>> cases{};
    std::optional<std::filesystem::path> anchor{};

    friend std::ostream&
    operator<<(std::ostream& os, const ignore_param& param) {
        std::ignore = param;
        return os;
    }
};

class ignore_test : public testing::TestWithParam<ignore_param> {};

// NOLINTNEXTLINE
TEST_P(ignore_test, test) {
    const auto& [globs, cases, anchor] = GetParam();

    auto actual = cases;
    for (auto& [node, ignored] : actual) {
        const glug::unit_test::temp_fs temp{};
        node.materialize(temp);
        const auto resolved_anchor = anchor ? temp / *anchor : temp;
        const auto filter = filter::ignore{ globs, resolved_anchor };
        ignored = filter.is_ignored(
                std::filesystem::directory_entry{ temp / node.leaf().path() }
        );
    }
    EXPECT_THAT(actual, testing::ElementsAreArray(cases));
}

static const auto ignore_cases = std::vector<ignore_param>{
    {
        { "dir_only/" },
        {
            { "dir_only"_f, decision::undecided },
            { "dir_only"_d, decision::excluded },
            { "dir"_d / "dir_only"_f, decision::undecided },
            { "dir"_d / "dir_only"_d, decision::excluded },
            // Files in ignored directories are not ignored explicitly,
            // but are ignored implicitly as they will not be enumerated.
            { "dir_only"_d / "file"_f, decision::undecided },
        },
    },
    {
        { "nofixup ", "fixup\\ " },
        {
            { "nofixup"_f, decision::excluded },
            { "nofixup "_f, decision::undecided },
            { "fixup"_f, decision::undecided },
            { "fixup "_f, decision::excluded },
        },
    },
    {
        { "mid space", "escaped\\ space" },
        {
            { "mid space"_f, decision::excluded },
            { "escaped space"_f, decision::excluded },
            { "escaped\\ space"_f, decision::undecided },
        },
    },
    {
        { "mid,comma", "escaped\\,comma" },
        {
            { "mid,comma"_f, decision::excluded },
            { "escaped,comma"_f, decision::excluded },
            { "escaped\\,comma"_f, decision::undecided },
        },
    },
    {
        { "file_only", "!file_only/" },
        {
            { "file_only"_f, decision::excluded },
            { "file_only"_d, decision::included },
            { "dir"_d / "file_only"_f, decision::excluded },
            { "dir"_d / "file_only"_d, decision::included },
        },
    },
    {
        { "anchored/exact" },
        {
            { "anchored"_d / "exact"_f, decision::excluded },
            { "sub"_d / "anchored"_d / "exact"_f, decision::undecided },
        },
    },
    {
        { "/anchored", "unanchored" },
        {
            { "sub"_d / "anchored"_f, decision::excluded },
            { "sub"_d / ("deeper"_d / "anchored"_f), decision::undecided },
            { "sub"_d / "unanchored"_f, decision::excluded },
            { "sub"_d / ("deeper"_d / "unanchored"_f), decision::excluded },
        },
        "sub",
    },
    {
        { "test_*", "!*.[ch]pp", "_*" },
        {
            { "README.md"_f, decision::undecided },
            { "test_data.txt"_f, decision::excluded },
            { "test_logic.cpp"_f, decision::included },
            { "test_logic.hpp"_f, decision::included },
            { "_test_data.generated.hpp"_f, decision::excluded },
        },
    },
    {
        { "*.[1-9]" },
        {
            { "a.0"_f, decision::undecided },
            { "a.1"_f, decision::excluded },
            { "a.2"_f, decision::excluded },
            { "a.8"_f, decision::excluded },
            { "a.9"_f, decision::excluded },
        },
    },
    {
        // https://github.com/python/cpython/issues/130942
        { "a[%-0]c" },
        {
            { "a.c"_f, decision::excluded },
            { "a"_d / "c"_f, decision::undecided },
        },
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(test, ignore_test, testing::ValuesIn(ignore_cases));

// NOLINTNEXTLINE
TEST(decision, to_string) {
    static constexpr auto str
            = [](auto x) { return (std::stringstream{} << x).str(); };

    EXPECT_EQ(str(decision::undecided), "undecided");
    EXPECT_EQ(str(decision::excluded), "excluded");
    EXPECT_EQ(str(decision::included), "included");
}

}  // namespace glug::filter::unit_test

