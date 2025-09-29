#include "glug/filesystem.hpp"

#include "glug/test/node.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <variant>
#include <vector>

using namespace glug::unit_test;

namespace glug::filesystem::unit_test {

struct explorer_param {
    node setup;
    node expected;
};

static std::ostream& operator<<(std::ostream& os, const explorer_param& param) {
    os << "{ ";
    for (const auto& path : param.setup.tree()) {
        os << std::filesystem::relative(path, node::temp_dir()) << ", ";
    }
    os << "}";
    return os;
}

class explorer_test : public testing::TestWithParam<explorer_param> {
    public:
    explorer_test() { std::filesystem::remove_all(node::temp_dir()); }
    ~explorer_test() { std::filesystem::remove_all(node::temp_dir()); }
};

TEST_F(explorer_test, iterators) {
    auto tree = node{ "iterators", { "README.md"_n } };
    auto exp = explorer{ tree.create().path() };

    EXPECT_EQ(exp.begin(), exp);
    EXPECT_NE(exp.begin(), explorer{});
    EXPECT_EQ(exp.end(), explorer{});
    EXPECT_NE(exp.end(), exp);
}

TEST_F(explorer_test, dereference) {
    auto tree = node{ "deref", { "LICENSE.txt", "README.md" } };
    auto exp = explorer{ tree.create().path() };

    const auto prefix = node::temp_dir();
    EXPECT_EQ((*exp).path(), prefix / "deref/LICENSE.txt");
    EXPECT_EQ(exp->path(), prefix / "deref/LICENSE.txt");
    EXPECT_EQ((exp++)->path(), prefix / "deref/LICENSE.txt");
    EXPECT_EQ(exp->path(), prefix / "deref/README.md");
}

TEST_P(explorer_test, test) {
    const auto& [setup, expected] = GetParam();

    auto exp = explorer{ setup.create().path() };
    const auto contents = std::vector(exp, exp.end());
    EXPECT_THAT(contents, testing::ElementsAreArray(expected.tree()));
}

// Directories with single files are ambiguous, need explicit types
static const auto explorer_cases = std::vector<explorer_param>{
    {
        { "simple", { "README.md"_n } },
        { "simple", { "README.md"_n } },
    },
    {
        {
            "with_gitignore",
            {
                "README.md",
                "build.log",
                { ".gitignore", "# no logs\n*.log" },
            },
        },
        {
            "with_gitignore",
            {
                ".gitignore",
                "README.md",
            },
        },
    },
    {
        {
            "nested",
            {
                "README.md",
                { ".gitignore", "*.log\n.cache/" },
                {
                    "src",
                    {
                        "main.c",
                        { ".gitignore", "*.generated.*" },
                        "main.generated.c",
                        "generated.log",
                    },
                },
                "build.log",
                ".cache/",
            },
        },
        {
            "nested",
            {
                ".gitignore",
                "README.md",
                {
                    "src",
                    {
                        ".gitignore",
                        "main.c",
                    },
                },
            },
        },
    },
    {
        { "empty_subdir", { "empty_dir/"_n } },
        { "empty_subdir/" },
    },
    {
        {
            "negate_ignore",
            {
                { ".gitignore", "*.zip" },
                "result.zip",
                {
                    "test",
                    {
                        { ".gitignore", "!data.zip" },
                        "data.zip",
                    },
                },
            },
        },
        {
            "negate_ignore",
            {
                ".gitignore",
                {
                    "test",
                    {
                        ".gitignore",
                        "data.zip",
                    },
                },
            },
        },
    },
    {
        {
            "all_ignored",
            {
                { ".gitignore", "generated/*.h" },
                {
                    "generated",
                    {
                        "foo.h",
                        "bar.h",
                    },
                },
            },
        },
        {
            "all_ignored",
            {
                ".gitignore"_n,
            },
        },
    },
    {
        {
            "anchored_tilde",
            {
                {
                    "weird~",
                    {
                        node{ ".gitignore", "/ignore.txt" },
                        "ignore.txt",
                        "include.txt",
                    },
                },
            },
        },
        {
            "anchored_tilde",
            {
                {
                    "weird~",
                    {
                        ".gitignore",
                        "include.txt",
                    },
                },
            },
        },
    },
    {
        {
            "anchored_brackets",
            {
                { ".gitignore", "[weird]" },
                {
                    "[weird]",
                    {
                        { ".gitignore", "/ignore.txt" },
                        "ignore.txt",
                        "include.txt",
                        "i",
                    },
                },
                "w",
                "e",
                "i",
                "r",
                "d",
                "o",
            },
        },
        {
            "anchored_brackets",
            {
                ".gitignore",
                "o",
                {
                    "[weird]",
                    {
                        ".gitignore",
                        "include.txt",
                    },
                },
            },
        },
    },
    {
        {
            "git_dir",
            {
                "README.md",
                {
                    ".git",
                    {
                        "HEAD"_n,
                    },
                },
            },
        },
        { "git_dir", { "README.md"_n } },
    }
};

INSTANTIATE_TEST_SUITE_P(
        explorer_test,
        explorer_test,
        testing::ValuesIn(explorer_cases),
        [](const auto& info) { return info.param.setup.name(); }
);

}  // namespace glug::filesystem::unit_test

