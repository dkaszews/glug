#include "glug/ignore.hpp"

#include "testing/parametrized.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace glug::ignore::unit_test {

using directory_entry = mocked<std::filesystem::directory_entry>;

struct filter_param {
    std::string source{};
    std::vector<std::string> globs{};
    std::map<directory_entry, decision> cases{};

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
    const auto list = filter{ globs, param.source };
    auto actual = param.cases;
    for (auto& [entry, ignored] : actual) {
        ignored = list.is_ignored(entry);
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
            { "dir/dir_only"_f, decision::undecided },
            { "dir/dir_only"_d, decision::ignored },
            // Files in ignored directories are not ignored explicitly,
            // but are ignored implicitly as they will not be enumerated.
            { "dir_only/file"_f, decision::undecided },
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
            { "dir/file_only"_f, decision::ignored },
            { "dir/file_only"_d, decision::included },
        },
    },
    {
        ".gitignore",
        { "anchored/exact" },
        {
            { "anchored/exact"_f, decision::ignored },
            { "sub/anchored/exact"_f, decision::undecided },
        },
    },
    {
        "sub/.gitignore",
        { "/anchored", "unanchored" },
        {
            { "sub/anchored"_f, decision::ignored },
            { "sub/deeper/anchored"_f, decision::undecided },
            { "sub/unanchored"_f, decision::ignored },
            { "sub/deeper/unanchored"_f, decision::ignored },
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
            { "a/c"_f, decision::undecided },
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

}  // namespace glug::ignore::unit_test

