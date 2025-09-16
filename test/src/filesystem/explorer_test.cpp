#include "glug/filesystem.hpp"

#include "glug/detail/mockable/access.hpp"
#include "glug/test/filesystem.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <variant>
#include <vector>

using namespace glug::unit_test;

namespace glug::filesystem::unit_test {

struct explorer_param {
    tree setup;
    tree expected;
};

static std::ostream& operator<<(std::ostream& os, const explorer_param& param) {
    os << "{ ";
    for (const auto& entry : param.setup.unwind()) {
        os << entry << ", ";
    }
    os << "}";
    return os;
}

class explorer_test : public testing::TestWithParam<explorer_param> {
    public:
#ifdef MOCK_FS
    std::filesystem::path prefix = "";
    testing::StrictMock<mockable<access>::mock> mock{};
#else
    std::filesystem::path prefix = create_temp_dir();
    ~explorer_test() { std::filesystem::remove_all(prefix); }
#endif
};

TEST_F(explorer_test, iterators) {
    ("iterators/" & tree::files{ "README.md" }).create(prefix);
    auto exp = explorer{ prefix / "iterators" };

    EXPECT_EQ(exp.begin(), exp);
    EXPECT_NE(exp.begin(), explorer{});
    EXPECT_EQ(exp.end(), explorer{});
    EXPECT_NE(exp.end(), exp);
}

TEST_F(explorer_test, dereference) {
    ("deref/" & tree::files{ "README.md", "LICENCE.txt" }).create(prefix);
    auto exp = explorer{ prefix / "deref" };

    EXPECT_EQ((*exp).path(), prefix / "deref/LICENCE.txt");
    EXPECT_EQ(exp->path(), prefix / "deref/LICENCE.txt");
    EXPECT_EQ((exp++)->path(), prefix / "deref/LICENCE.txt");
    EXPECT_EQ(exp->path(), prefix / "deref/README.md");
}

TEST_P(explorer_test, test) {
    const auto& [setup, expected] = GetParam();
    setup.create(prefix);

    // `parent_path` to get rid of trailing `/` we use to mark directories
    auto exp = explorer{ prefix / setup.path.parent_path() };
    const auto contents = std::vector(exp, exp.end());
    EXPECT_THAT(contents, testing::ElementsAreArray(expected.unwind(prefix)));
}

// Clang format seems confused by operator overloading, need to format manually
static const auto explorer_cases = std::vector<explorer_param>{
    {
        "simple/" & tree::files{ "README.md" },
        "simple/" & tree::files{ "README.md" },
    },
    {
        "with_gitignore/" & tree::files{
            "README.md",
            "build.log",
            ".gitignore" & tree::lines{ "# no logs", "*.log" },
        },
        "with_gitignore/" & tree::files{
            ".gitignore",
            "README.md",
        },
    },
    {
        "nested/" & tree::files{
            "README.md" ,
            ".gitignore" & tree::lines{ "*.log", ".cache/" },
            "src/" & tree::files{
                "main.c",
                ".gitignore" & tree::lines{ "*.generated.*" },
                "main.generated.c",
                "generated.log",
            },
            "build.log",
            ".cache/",
        },
        "nested/" & tree::files{
            ".gitignore",
            "README.md" ,
            "src/" & tree::files{
                ".gitignore",
                "main.c",
            },
        },
    },
    {
        "empty_subdir/" & tree::files{
            "empty_dir/" & tree::files{}
        },
        tree{ "/" },
    },
    {
        "negate_ignore/" & tree::files{
            ".gitignore" & tree::lines{ "*.zip" },
            "result.zip",
            "test/" & tree::files{
                ".gitignore" & tree::lines{ "!data.zip" },
                "data.zip",
            },
        },
        "negate_ignore/" & tree::files{
            ".gitignore",
            "test/" & tree::files{
                ".gitignore",
                "data.zip",
            },
        },
    },
    {
        "all_ignored/" & tree::files{
            ".gitignore" & tree::lines{ "generated/*.h" },
            "generated/" & tree::files{
                "foo.h",
                "bar.h",
            },
        },
        "all_ignored/" & tree::files{
            ".gitignore",
        },
    },
    {
        "anchored_tilde/" & tree::files{
            "weird~/" & tree::files{
                ".gitignore" & tree::lines{ "/ignore.txt" },
                "ignore.txt",
                "include.txt",
            }
        },
        "anchored_tilde/" & tree::files{
            "weird~/" & tree::files{
                ".gitignore",
                "include.txt",
            }
        },
    },
    {
        "anchored_brackets/" & tree::files{
            ".gitignore" & tree::lines{ "[weird]" },
            "[weird]/" & tree::files{
                ".gitignore" & tree::lines{ "/ignore.txt" },
                "ignore.txt",
                "include.txt",
            },
            "w",
            "e",
            "i",
            "r",
            "d",
            "o",
        },
        "anchored_brackets/" & tree::files{
            ".gitignore",
            "o",
            "[weird]/" & tree::files{
                ".gitignore",
                "include.txt",
            }
        },
    },
};

INSTANTIATE_TEST_SUITE_P(
        explorer_test,
        explorer_test,
        testing::ValuesIn(explorer_cases),
        [](const auto& info) {
            return info.param.setup.path.parent_path().string();
        }
);

}  // namespace glug::filesystem::unit_test

