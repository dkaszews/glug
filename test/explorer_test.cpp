#include "glug/filesystem.hpp"

#include "testing/filesystem.hpp"

#include <gtest/gtest.h>

#include <variant>
#include <vector>

using namespace glug::unit_test;

namespace glug::filesystem::unit_test {

struct explorer_param {
    test_fs tree;
    test_fs expected;
};

static std::ostream& operator<<(std::ostream& os, const explorer_param& param) {
    os << "{ ";
    for (const auto& entry : param.tree.unwind()) {
        os << entry << ", ";
    }
    os << "}";
    return os;
}

class explorer_test : public testing::TestWithParam<explorer_param> {
    public:
    testing::StrictMock<access_mock> mock{};
};

TEST_F(explorer_test, iterators) {
    ("/"_d / test_fs::files{ "README.md"_f }).create(mock);
    auto exp = explorer{ "/" };

    EXPECT_EQ(exp.begin(), exp);
    EXPECT_NE(exp.begin(), explorer{});
    EXPECT_EQ(exp.end(), explorer{});
    EXPECT_NE(exp.end(), exp);
}

TEST_F(explorer_test, dereference) {
    ("/"_d / test_fs::files{ "README.md"_f, "LICENCE.txt"_f }).create(mock);
    auto exp = explorer{ "/" };

    EXPECT_EQ((*exp).path(), "/LICENCE.txt");
    EXPECT_EQ(exp->path(), "/LICENCE.txt");
    EXPECT_EQ((exp++)->path(), "/LICENCE.txt");
    EXPECT_EQ(exp->path(), "/README.md");
}

TEST_P(explorer_test, test) {
    const auto& [tree, expected] = GetParam();
    tree.create(mock);

    auto exp = explorer{ tree.entry.path() };
    const auto contents = std::vector(exp, exp.end());
    EXPECT_THAT(contents, testing::ElementsAreArray(expected.unwind()));
}

// Clang format seems confused by operator overloading, need to format manually
static const auto explorer_cases = std::vector<explorer_param>{
    {
        "simple"_d / test_fs::files{ "README.md"_f },
        "simple"_d / test_fs::files{ "README.md"_f },
    },
    {
        "with_gitignore"_d / test_fs::files{
            "README.md"_f,
            "build.log"_f,
            ".gitignore"_f / test_fs::lines{ "# no logs", "*.log" },
        },
        "with_gitignore"_d / test_fs::files{
            ".gitignore"_f,
            "README.md"_f,
        },
    },
    {
        "nested"_d / test_fs::files{
            "README.md"_f ,
            ".gitignore"_f / test_fs::lines{ "*.log", ".cache/" },
            "src"_d / test_fs::files{
                "main.c"_f,
                ".gitignore"_f / test_fs::lines{ "*.generated.*" },
                "main.generated.c"_f,
                "generated.log"_f,
            },
            "build.log"_f,
            ".cache"_d,
        },
        "nested"_d / test_fs::files{
            ".gitignore"_f,
            "README.md"_f ,
            "src"_d / test_fs::files{
                ".gitignore"_f,
                "main.c"_f,
            },
        },
    },
    {
        "empty_subdir"_d / test_fs::files{
            "empty_dir"_d / test_fs::files{}
        },
        test_fs{ ""_d },
    },
    {
        "negate_ignore"_d / test_fs::files{
            ".gitignore"_f / test_fs::lines{ "*.zip" },
            "result.zip"_f,
            "test/"_d / test_fs::files{
                ".gitignore"_f / test_fs::lines{ "!data.zip" },
                "data.zip"_f,
            },
        },
        "negate_ignore"_d / test_fs::files{
            ".gitignore"_f,
            "test/"_d / test_fs::files{
                ".gitignore"_f,
                "data.zip"_f,
            },
        },
    },
    {
        "all_ignored"_d / test_fs::files{
            ".gitignore"_f / test_fs::lines{ "generated/*.h" },
            "generated"_d / test_fs::files{
                "foo.h"_f,
                "bar.h"_f,
            },
        },
        "all_ignored"_d / test_fs::files{
            ".gitignore"_f,
        },
    },
};

INSTANTIATE_TEST_SUITE_P(
        explorer_test,
        explorer_test,
        testing::ValuesIn(explorer_cases),
        [](const auto& info) { return info.param.tree.entry.path().string(); }
);

}  // namespace glug::filesystem::unit_test

