// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filter.hpp"

#include "parametrized.hpp"
#include "tree.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace glug::glob::unit_test {

using glug::unit_test::tree;

struct filter_param {
    std::string source{};
    std::vector<std::string> globs{};
    std::map<tree, decision> cases{};

    friend void PrintTo(const filter_param& param, std::ostream* os) {
        std::ignore = std::tie(param, os);
    }
};

class filter_test : public testing::TestWithParam<filter_param> {
    public:
    filter_test() { std::filesystem::remove_all(tree::temp_dir()); }
    ~filter_test() { std::filesystem::remove_all(tree::temp_dir()); }
};

TEST_P(filter_test, test) {
    const auto& param = GetParam();
    const auto globs = std::vector<std::string_view>{
        param.globs.begin(),
        param.globs.end(),
    };
    const auto list = filter{ globs, tree::temp_dir() / param.source };
    auto actual = param.cases;
    for (auto& [descriptor, ignored] : actual) {
        ignored = list.is_ignored(descriptor.create());
        descriptor.remove();
    }
    EXPECT_THAT(actual, testing::ElementsAreArray(param.cases));
}

static const auto filter_cases = std::vector<filter_param>{
    {
        ".gitignore",
        { "dir_only/" },
        {
            { "dir_only", decision::undecided },
            { "dir_only/", decision::ignored },
            { "dir/dir_only", decision::undecided },
            { "dir/dir_only/", decision::ignored },
            // Files in ignored directories are not ignored explicitly,
            // but are ignored implicitly as they will not be enumerated.
            { "dir_only/file", decision::undecided },
        },
    },
    {
        ".gitignore",
        { "nofixup ", "fixup\\ " },
        {
            { "nofixup", decision::ignored },
            { "nofixup ", decision::undecided },
            { "fixup", decision::undecided },
            { "fixup ", decision::ignored },
        },
    },
    {
        ".gitignore",
        { "file_only", "!file_only/" },
        {
            { "file_only", decision::ignored },
            { "file_only/", decision::included },
            { "dir/file_only", decision::ignored },
            { "dir/file_only/", decision::included },
        },
    },
    {
        ".gitignore",
        { "anchored/exact" },
        {
            { "anchored/exact", decision::ignored },
            { "sub/anchored/exact", decision::undecided },
        },
    },
    {
        "sub/.gitignore",
        { "/anchored", "unanchored" },
        {
            { "sub/anchored", decision::ignored },
            { "sub/deeper/anchored", decision::undecided },
            { "sub/unanchored", decision::ignored },
            { "sub/deeper/unanchored", decision::ignored },
        },
    },
    {
        ".gitignore",
        { "test_*", "!*.[ch]pp", "_*" },
        {
            { "README.md", decision::undecided },
            { "test_data.txt", decision::ignored },
            { "test_logic.cpp", decision::included },
            { "test_logic.hpp", decision::included },
            { "_test_data.generated.hpp", decision::ignored },
        },
    },
    {
        ".gitignore",
        { "*.[1-9]" },
        {
            { "a.0", decision::undecided },
            { "a.1", decision::ignored },
            { "a.2", decision::ignored },
            { "a.8", decision::ignored },
            { "a.9", decision::ignored },
        },
    },
    {
        // https://github.com/python/cpython/issues/130942
        ".gitignore",
        { "a[%-0]c" },
        {
            { "a.c", decision::ignored },
            { "a/c", decision::undecided },
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

