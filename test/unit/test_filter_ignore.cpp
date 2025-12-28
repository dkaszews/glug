// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filter.hpp"

#include "tree.hpp"

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glug::filter::unit_test {

using glug::unit_test::operator""_d;
using glug::unit_test::operator""_f;

struct filter_param {
    std::filesystem::path source{};
    std::vector<std::string> globs{};
    std::vector<std::pair<glug::unit_test::node, decision>> cases{};

    std::ostream& operator<<(std::ostream& os) const { return os; }
};

class filter_test : public testing::TestWithParam<filter_param> {};

// NOLINTNEXTLINE
TEST_P(filter_test, test) {
    const auto& param = GetParam();
    const auto globs = std::vector<std::string_view>{
        param.globs.begin(),
        param.globs.end(),
    };

    auto actual = param.cases;
    for (auto& [node, ignored] : actual) {
        const glug::unit_test::temp_fs temp{};
        node.materialize(temp);
        const auto filter = filter::ignore{ globs, temp / param.source };
        ignored = filter.is_ignored(
                std::filesystem::directory_entry{ temp / node.leaf().path() }
        );
    }
    EXPECT_THAT(actual, testing::ElementsAreArray(param.cases));
}

static const auto filter_cases = std::vector<filter_param>{
    {
        ".gitignore",
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
        ".gitignore",
        { "nofixup ", "fixup\\ " },
        {
            { "nofixup"_f, decision::excluded },
            { "nofixup "_f, decision::undecided },
            { "fixup"_f, decision::undecided },
            { "fixup "_f, decision::excluded },
        },
    },
    {
        ".gitignore",
        { "mid space", "escaped\\ space" },
        {
            { "mid space"_f, decision::excluded },
            { "escaped space"_f, decision::excluded },
            { "escaped\\ space"_f, decision::undecided },
        },
    },
    {
        ".gitignore",
        { "mid,comma", "escaped\\,comma" },
        {
            { "mid,comma"_f, decision::excluded },
            { "escaped,comma"_f, decision::excluded },
            { "escaped\\,comma"_f, decision::undecided },
        },
    },
    {
        ".gitignore",
        { "file_only", "!file_only/" },
        {
            { "file_only"_f, decision::excluded },
            { "file_only"_d, decision::included },
            { "dir"_d / "file_only"_f, decision::excluded },
            { "dir"_d / "file_only"_d, decision::included },
        },
    },
    {
        ".gitignore",
        { "anchored/exact" },
        {
            { "anchored"_d / "exact"_f, decision::excluded },
            { "sub"_d / "anchored"_d / "exact"_f, decision::undecided },
        },
    },
    {
        "sub/.gitignore",
        { "/anchored", "unanchored" },
        {
            { "sub"_d / "anchored"_f, decision::excluded },
            { "sub"_d / ("deeper"_d / "anchored"_f), decision::undecided },
            { "sub"_d / "unanchored"_f, decision::excluded },
            { "sub"_d / ("deeper"_d / "unanchored"_f), decision::excluded },
        },
    },
    {
        ".gitignore",
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
        ".gitignore",
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
        ".gitignore",
        { "a[%-0]c" },
        {
            { "a.c"_f, decision::excluded },
            { "a"_d / "c"_f, decision::undecided },
        },
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(test, filter_test, testing::ValuesIn(filter_cases));

// NOLINTNEXTLINE
TEST(decision, to_string) {
    static constexpr auto str
            = [](auto x) { return (std::stringstream{} << x).str(); };

    EXPECT_EQ(str(decision::undecided), "undecided");
    EXPECT_EQ(str(decision::excluded), "excluded");
    EXPECT_EQ(str(decision::included), "included");
}

}  // namespace glug::filter::unit_test

