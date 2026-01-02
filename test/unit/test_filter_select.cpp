// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#include "glug/filter.hpp"

#include "tree.hpp"

#include <filesystem>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glug::filter::unit_test {

using glug::unit_test::operator""_d;
using glug::unit_test::operator""_f;

struct select_param {
    std::string globs{};
    std::vector<std::pair<glug::unit_test::node, decision>> cases{};
    std::filesystem::path anchor{};

    friend std::ostream&
    operator<<(std::ostream& os, const select_param& param) {
        return os << param.globs;
    }
};

class select_test : public testing::TestWithParam<select_param> {};

// NOLINTNEXTLINE
TEST_P(select_test, test) {
    const auto& param = GetParam();
    auto actual = param.cases;
    for (auto& [node, decision] : actual) {
        const glug::unit_test::temp_fs temp{};
        node.materialize(temp);
        const auto anchor = !param.anchor.empty() ? temp / param.anchor : temp;
        const auto filter = filter::select{ param.globs, anchor };
        decision = filter(
                std::filesystem::directory_entry{ temp / node.leaf().path() }
        );
    }
    EXPECT_THAT(actual, testing::ElementsAreArray(param.cases));
}

static const auto select_cases = std::vector<select_param>{
    {
        "",
        {
            { "README.md"_f, decision::undecided },
            { "main.cpp"_f, decision::undecided },
            { "src"_d, decision::undecided },
        },
    },
    {
        "*.md",
        {
            { "README.md"_f, decision::included },
            { "README.md"_d, decision::undecided },
            { "readme.md"_f, decision::included },
            { "main.cpp"_f, decision::excluded },
        },
    },
    {
        "*,-*.md",
        {
            { "README.md"_f, decision::excluded },
            { "README.md"_d, decision::undecided },
            { "main.cpp"_f, decision::included },
            { "foo.hpp"_f, decision::included },
        },
    },
    {
        "-*.md",
        {
            { "README.md"_f, decision::excluded },
            { "README.md"_d, decision::undecided },
            { "main.cpp"_f, decision::undecided },
            { "foo.hpp"_f, decision::undecided },
        },
    },
    {
        "*.cpp,*.hpp,-main.*",
        {
            { "main.cpp"_f, decision::excluded },
            { "main.log"_f, decision::excluded },
            { "foo.cpp"_f, decision::included },
            { "foo.hpp"_f, decision::included },
            { "README.md"_f, decision::excluded },
        },
    },
    {
        "src/",
        {
            { "src"_d, decision::included },
            { "extra"_d / "src"_d, decision::included },
            { "extra"_d, decision::excluded },
            { "include"_d, decision::excluded },
            { "README.md"_f, decision::undecided },
        },
    },
    {
        "src/*.cpp",
        {
            { "src"_d, decision::undecided },
            { "src"_d / "lib.cpp"_f, decision::included },
            { "src"_d / ("detail"_d / "impl.cpp"_f), decision::excluded },
            { "extra"_d / ("src"_d / "extra.cpp"_f), decision::excluded },
            { "main.cpp"_f, decision::excluded },
        },
    },
    {
        "src/**/*.cpp",
        {
            { "src"_d, decision::undecided },
            { "src"_d / "lib.cpp"_f, decision::included },
            { "src"_d / ("detail"_d / "impl.cpp"_f), decision::included },
            { "extra"_d / ("src"_d / "extra.cpp"_f), decision::excluded },
            { "main.cpp"_f, decision::excluded },
        },
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(test, select_test, testing::ValuesIn(select_cases));

}  // namespace glug::filter::unit_test

