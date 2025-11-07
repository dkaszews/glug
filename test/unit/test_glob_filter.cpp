// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filter.hpp"

#include "parametrized.hpp"
#include "tree.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace glug::glob::unit_test {

using namespace glug::unit_test;

struct filter_param {
    std::filesystem::path source{};
    std::vector<std::string> globs{};
    std::vector<std::pair<node, decision>> cases{};

    friend void PrintTo(const filter_param& param, std::ostream* os) {
        std::ignore = std::tie(param, os);
    }
};

class filter_test : public testing::TestWithParam<filter_param> {};

TEST_P(filter_test, test) {
    const auto& param = GetParam();
    const auto globs = std::vector<std::string_view>{
        param.globs.begin(),
        param.globs.end(),
    };

    auto actual = param.cases;
    for (auto& [node, ignored] : actual) {
        const temp_fs temp{};
        node.materialize(temp);
        const auto list = filter{ globs, temp / param.source };
        ignored = list.is_ignored(
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
            { "dir_only"_d, decision::ignored },
            { "dir"_d / "dir_only"_f, decision::undecided },
            { "dir"_d / "dir_only"_d, decision::ignored },
            // // Files in ignored directories are not ignored explicitly,
            // // but are ignored implicitly as they will not be enumerated.
            { "dir_only"_d / "file"_f, decision::undecided },
        },
    },
    {
        ".gitignore",
        { "nofixup ", "fixup\\ " },
        {
            { "nofixup"_f, decision::ignored },
            { "nofixup "_f, decision::undecided },
            { "fixup"_f, decision::undecided },
            { "fixup "_f, decision::ignored },
        },
    },
    {
        ".gitignore",
        { "file_only", "!file_only/" },
        {
            { "file_only"_f, decision::ignored },
            { "file_only"_d, decision::included },
            { "dir"_d / "file_only"_f, decision::ignored },
            { "dir"_d / "file_only"_d, decision::included },
        },
    },
    {
        ".gitignore",
        { "anchored/exact" },
        {
            { "anchored"_d / "exact"_f, decision::ignored },
            { "sub"_d / "anchored"_d / "exact"_f, decision::undecided },
        },
    },
    {
        "sub/.gitignore",
        { "/anchored", "unanchored" },
        {
            { "sub"_d / "anchored"_f, decision::ignored },
            { "sub"_d / ("deeper"_d / "anchored"_f), decision::undecided },
            { "sub"_d / "unanchored"_f, decision::ignored },
            { "sub"_d / ("deeper"_d / "unanchored"_f), decision::ignored },
        },
    },
    {
        ".gitignore",
        { "test_*", "!*.[ch]pp", "_*" },
        {
            { "README.md"_f, decision::undecided },
            { "test_data.txt"_f, decision::ignored },
            { "test_logic.cpp"_f, decision::included },
            { "test_logic.hpp"_f, decision::included },
            { "_test_data.generated.hpp"_f, decision::ignored },
        },
    },
    {
        ".gitignore",
        { "*.[1-9]" },
        {
            { "a.0"_f, decision::undecided },
            { "a.1"_f, decision::ignored },
            { "a.2"_f, decision::ignored },
            { "a.8"_f, decision::ignored },
            { "a.9"_f, decision::ignored },
        },
    },
    {
        // https://github.com/python/cpython/issues/130942
        ".gitignore",
        { "a[%-0]c" },
        {
            { "a.c"_f, decision::ignored },
            { "a"_d / "c"_f, decision::undecided },
        },
    },
};

INSTANTIATE_TEST_SUITE_P(test, filter_test, testing::ValuesIn(filter_cases));

TEST(decision, to_string) {
    static constexpr auto str
            = [](auto x) { return (std::stringstream{} << x).str(); };

    EXPECT_EQ(str(decision::undecided), "undecided");
    EXPECT_EQ(str(decision::ignored), "ignored");
    EXPECT_EQ(str(decision::included), "included");
}

}  // namespace glug::glob::unit_test

