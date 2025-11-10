// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filesystem.hpp"

#include "tree.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <variant>
#include <vector>

namespace glug::filesystem::unit_test {

using namespace glug::unit_test;

struct explorer_param {
    node tree;
    std::vector<std::filesystem::path> expected{};

    friend void PrintTo(const explorer_param& param, std::ostream* os) {
        std::ignore = std::tie(param, os);
    }
};

class explorer_test : public testing::TestWithParam<explorer_param> {};

TEST_F(explorer_test, iterators) {
    auto tree = "iterators"_d / "README.md"_f;
    const auto temp = temp_fs{};
    tree.materialize(temp);
    auto exp = explorer{ temp / tree.path() };

    EXPECT_EQ(exp.begin(), exp);
    EXPECT_NE(exp.begin(), explorer{});
    EXPECT_EQ(exp.end(), explorer{});
    EXPECT_NE(exp.end(), exp);
}

TEST_F(explorer_test, dereference) {
    auto tree = dir{ "deref", { "LICENSE.txt"_f, "README.md"_f } };
    const auto temp = temp_fs{};
    tree.materialize(temp);
    auto exp = explorer{ temp / tree.path() };

    const auto& prefix = temp.path();
    EXPECT_EQ((*exp).path(), prefix / "deref/LICENSE.txt");
    EXPECT_EQ(exp->path(), prefix / "deref/LICENSE.txt");
    EXPECT_EQ((exp++)->path(), prefix / "deref/LICENSE.txt");
    EXPECT_EQ(exp->path(), prefix / "deref/README.md");
}

TEST_P(explorer_test, test) {
    const auto& [tree, expected] = GetParam();
    const auto temp = temp_fs{};
    try {
        tree.materialize(temp);
    } catch (const std::filesystem::filesystem_error& e) {
        GTEST_SKIP() << e.what();
    }

    auto exp = explorer{ temp / tree.path() };
    auto relative = std::vector<std::filesystem::path>{};
    std::transform(
            exp,
            exp.end(),
            std::back_inserter(relative),
            [&temp](const auto& entry) {
                return std::filesystem::relative(entry, temp);
            }
    );
    EXPECT_THAT(relative, testing::ElementsAreArray(expected));
}

static const auto explorer_cases = std::vector<explorer_param>{
    {
        { "simple"_d / "README.md"_f },
        { "simple/README.md" },
    },
    {
        dir{
            "with_gitignore",
            {
                file{ "README.md" },
                file{ "build.log" },
                file{ ".gitignore", "# no logs\n*.log" },
            },
        },
        {
            "with_gitignore/.gitignore",
            "with_gitignore/README.md",
        },
    },
    {
        dir{
            "nested",
            {
                file{ "README.md" },
                file{ ".gitignore", "*.log\n.cache/" },
                dir{
                    "src",
                    {
                        file{ "main.c" },
                        file{ ".gitignore", "*.generated.*" },
                        file{ "main.generated.c" },
                        file{ "generated.log" },
                    },
                },
                file{ "build.log" },
                dir{ ".cache" } / file{ "main.c.obj" },
            },
        },
        {
            "nested/.gitignore",
            "nested/README.md",
            "nested/src/.gitignore",
            "nested/src/main.c",
        },
    },
    {
        "empty_subdir"_d / "empty_dir"_d,
        {},
    },
    {
        dir{
            "negate_ignore",
            {
                file{ ".gitignore", "*.zip" },
                file{ "result.zip" },
                dir{
                    "test",
                    {
                        file{ ".gitignore", "!data.zip" },
                        file{ "data.zip" },
                    },
                },
            },
        },
        {
            "negate_ignore/.gitignore",
            "negate_ignore/test/.gitignore",
            "negate_ignore/test/data.zip",
        },
    },
    {
        dir{
            "all_ignored",
            {
                file{ ".gitignore", "generated/*.h" },
                dir{
                    "generated",
                    {
                        "foo.h"_f,
                        "bar.h"_f,
                    },
                },
            },
        },
        {
            "all_ignored/.gitignore",
        },
    },
    {
        dir{
            "anchored_tilde",
            {
                dir{
                    "weird~",
                    {
                        file{ ".gitignore", "/ignore.txt" },
                        file{ "ignore.txt" },
                        file{ "include.txt" },
                    },
                },
            },
        },
        {
            "anchored_tilde/weird~/.gitignore",
            "anchored_tilde/weird~/include.txt",
        },
    },
    {
        dir{
            "anchored_brackets",
            {
                file{ ".gitignore", "[weird]" },
                dir{
                    "[weird]",
                    {
                        file{ ".gitignore", "/ignore.txt" },
                        file{ "ignore.txt" },
                        file{ "include.txt" },
                        file{ "i" },
                    },
                },
                file{ "w" },
                file{ "e" },
                file{ "i" },
                file{ "r" },
                file{ "d" },
                file{ "o" },
            },
        },
        {
            "anchored_brackets/.gitignore",
            "anchored_brackets/o",
            "anchored_brackets/[weird]/.gitignore",
            "anchored_brackets/[weird]/include.txt",
        },
    },
    {
        dir{
            "git_dir",
            {
                "README.md"_f,
                ".git"_d / "HEAD"_f,
            },
        },
        {
            "git_dir/README.md",
        },
    },
    {
        dir{
            "symlinks",
            {
                dir{
                    "docs",
                    {
                        file{ "README.md" },
                    },
                },
                link{ "documentation", "docs" },
                link{ "README.md", "docs/README.md" },
            },
        },
        {
            "symlinks/docs/README.md",
        },
    },
    {
        dir{
            "linux_v2_6_39",
            {
                "arch"_d / ("microblaze"_d / "boot"_d),
                "block"_d,
            },
        },
        {},
    },
};

INSTANTIATE_TEST_SUITE_P(
        explorer_test,
        explorer_test,
        testing::ValuesIn(explorer_cases),
        [](const auto& info) { return info.param.tree.name().string(); }
);

}  // namespace glug::filesystem::unit_test

