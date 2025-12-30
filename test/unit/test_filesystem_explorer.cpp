// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filesystem.hpp"

#include "tree.hpp"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <optional>
#include <ostream>
#include <string_view>
#include <tuple>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glug::filesystem::unit_test {

// Moving into outer scope causes ambiguity of `link` with linux function
using glug::unit_test::dir;
using glug::unit_test::file;
using glug::unit_test::link;
using glug::unit_test::temp_fs;
using glug::unit_test::operator""_d;
using glug::unit_test::operator""_f;

struct explorer_param {
    glug::unit_test::node tree;
    std::vector<std::filesystem::path> expected{};
    std::optional<std::filesystem::path> target{};
    std::optional<std::string_view> select{};

    friend std::ostream&
    operator<<(std::ostream& os, const explorer_param& param) {
        std::ignore = param;
        return os;
    }
};

class explorer_test : public testing::TestWithParam<explorer_param> {};

// NOLINTNEXTLINE
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

// NOLINTNEXTLINE
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

// NOLINTNEXTLINE
TEST_P(explorer_test, test) {
    const auto& [tree, expected, target, select] = GetParam();
    const auto temp = temp_fs{};
    try {
        tree.materialize(temp);
    } catch (const std::filesystem::filesystem_error& e) {
        GTEST_SKIP() << e.what();
    }

    const auto resolved_target = temp / target.value_or(tree.path());
    const auto options = explorer_options{
        filter::select_filter{ select.value_or(""), resolved_target },
    };
    auto exp = explorer{ resolved_target, options };
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
                file{ ".gitignore", "# no logs\n\n*.log" },
            },
        },
        {
            "with_gitignore/.gitignore",
            "with_gitignore/README.md",
        },
    },
    {
        dir{
            "with_gitignore_crlf",
            {
                file{ "README.md" },
                file{ "build.log" },
                file{ ".gitignore", "# no logs\r\n\r\n*.log\r\n" },
            },
        },
        {
            "with_gitignore_crlf/.gitignore",
            "with_gitignore_crlf/README.md",
        },
    },
    {
        dir{
            "unicode_name",
            {
                dir{
                    "translations",
                    {
                        "українська.md"_f,
                        "Ελληνική.md"_f,
                        "한국어.md"_f,
                        "generated.md"_f,
                        "generate.sh"_f,
                    },
                },
                file{ ".gitignore", "generated*" },
            },
        },
        {
            "unicode_name/.gitignore",
            "unicode_name/translations/generate.sh",
            "unicode_name/translations/Ελληνική.md",
            "unicode_name/translations/українська.md",
            "unicode_name/translations/한국어.md",
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
            "recurse_empty_after_empty",
            {
                "a"_d / ("b"_d / "c"_d),
                "x"_d,
            },
        },
        {},
    },
    {
        dir{
            "recurse_nonempty_after_empty",
            {
                "a"_d / ("b"_d / "c"_d),
                "x"_d / ("y"_d / "z"_f),
            },
        },
        {
            "recurse_nonempty_after_empty/x/y/z",
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
            "outer",
            {
                file{ ".gitignore", "*.log\n*.zip" },
                dir{
                    "middle",
                    {
                        file{ ".gitignore", "!*.zip" },
                        dir{
                            "inner",
                            {
                                "out.log"_f,
                                "README.md"_f,
                                "thingy.zip"_f,
                            },
                        },
                    },
                },
            },
        },
        {
            "outer/middle/inner/README.md",
            "outer/middle/inner/thingy.zip",
        },
        "outer/middle/inner",
    },
    {
        dir{
            "outer_with_git_barrier",
            {
                file{ ".gitignore", "*.log" },
                dir{
                    "middle",
                    {
                        dir{ ".git" },
                        dir{
                            "inner",
                            {
                                "out.log"_f,
                                "README.md"_f,
                            },
                        },
                    },
                },
            },
        },
        {
            "outer_with_git_barrier/middle/inner/README.md",
            "outer_with_git_barrier/middle/inner/out.log",
        },
        "outer_with_git_barrier/middle/inner",
    },
    {
        dir{
            "repo_with_submodule",
            {
                dir{ ".git" },
                file{ ".gitignore", "*.log" },
                file{ "excluded.log" },
                file{ "included.txt" },
                dir{
                    "submodules",
                    {
                        dir{ ".git" },
                        file{ ".gitignore", "*.txt" },
                        file{ "excluded.txt" },
                        file{ "included.log" },
                    },
                },
            },
        },
        {
            "repo_with_submodule/.gitignore",
            "repo_with_submodule/included.txt",
        },
    },
    {
        dir{
            "projects_directory",
            {
                file{ ".gitignore", "*.log" },
                dir{
                    "first",
                    {
                        dir{ ".git" },
                        file{ ".gitignore", "*.log" },
                        file{ "README.md" },
                        file{ "excluded.log" },
                    },
                },
                dir{
                    "second",
                    {
                        dir{ ".git" },
                        file{ "README.md" },
                        file{ "included.log" },
                    },
                },
                dir{
                    "third",
                    {
                        dir{ ".git" },
                        file{ "README.md" },
                        dir{
                            "submodules",
                            {
                                dir{ ".git" },
                                file{ "README.md" },
                            },
                        },
                    },
                },
            },
        },
        {
            "projects_directory/.gitignore",
            "projects_directory/first/.gitignore",
            "projects_directory/first/README.md",
            "projects_directory/second/README.md",
            "projects_directory/second/included.log",
            "projects_directory/third/README.md",
        },
    },
    {
        dir{
            "submodule_target_middle",
            {
                dir{ ".git" },
                file{ "README.md" },
                dir{
                    "submodules",
                    {
                        file{ "README.md" },
                        dir{
                            "dependency",
                            {
                                ".git"_d,
                                "README.md"_f,
                            },
                        },
                    },
                },
            },
        },
        {
            "submodule_target_middle/submodules/README.md",
        },
        "submodule_target_middle/submodules",
    },
    {
        dir{
            "select_cpp",
            {
                file{ ".gitignore", "*.generated.*" },
                dir{
                    "src",
                    {
                        "main.cpp"_f,
                        "foo.cpp"_f,
                    },
                },
                dir{
                    "include",
                    {
                        "foo.hpp"_f,
                        "foo.generated.hpp"_f,
                    },
                },
            },
        },
        {
            "select_cpp/include/foo.hpp",
            "select_cpp/src/foo.cpp",
        },
        std::nullopt,
        "*.cpp,*.hpp,-main.*",
    },
    {
        dir{
            "select_dir",
            {
                file{ ".gitignore", "*.log" },
                dir{
                    "test",
                    {
                        "data"_d / "curl.py"_f,
                        "run.py"_f,
                        "results.log"_f,
                    },
                },
                file{ "run_tests.py" },
            },
        },
        {
            // Selecting directory does not prevent searching root
            "select_dir/.gitignore",
            "select_dir/run_tests.py",
            "select_dir/test/run.py",
        },
        std::nullopt,
        "test/",
    },
    {
        dir{
            "select_dir_content",
            {
                file{ ".gitignore", "*.log" },
                dir{
                    "test",
                    {
                        "data"_d / "curl.py"_f,
                        "run.py"_f,
                        "results.log"_f,
                    },
                },
                file{ "run_tests.py" },
            },
        },
        {
            "select_dir_content/test/run.py",
        },
        std::nullopt,
        "test/*",
    },
    {
        dir{
            "select_dir_content_recursive",
            {
                file{ ".gitignore", "*.log" },
                dir{
                    "test",
                    {
                        "data"_d / "curl.py"_f,
                        "run.py"_f,
                        "results.log"_f,
                    },
                },
                file{ "run_tests.py" },
            },
        },
        {
            "select_dir_content_recursive/test/run.py",
            "select_dir_content_recursive/test/data/curl.py",
        },
        std::nullopt,
        "test/**/*",
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        explorer_test,
        explorer_test,
        testing::ValuesIn(explorer_cases),
        [](const auto& info) { return info.param.tree.name().string(); }
);

}  // namespace glug::filesystem::unit_test

