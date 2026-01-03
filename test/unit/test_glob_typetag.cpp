// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/glob.hpp"

#include "parametrized.hpp"

#include <ostream>
#include <string_view>
#include <tuple>

#include <gtest/gtest.h>

namespace glug::glob::unit_test {

struct typetag_param {
    std::string_view globs = {};
    std::vector<std::string_view> expected = {};

    friend std::ostream&
    operator<<(std::ostream& os, const typetag_param& param) {
        std::ignore = param;
        return os;
    }
};

class typetag_test : public testing::TestWithParam<typetag_param> {};

// NOLINTNEXTLINE
TEST_P(typetag_test, test) {
    const auto db = typetag_database{
        { "cpp", "*.cpp,*.cxx,*.hpp,*.hxx" },
        { "hpp", "*.hpp,*.hxx" },
        { "c", "*.c,*.h" },
    };

    const auto& [globs, expected] = GetParam();
    EXPECT_EQ(db.expand(globs), expected);
}

const auto typetag_cases = std::vector<typetag_param>{
    {
        "",
        {},
    },
    {
        "*",
        { "*" },
    },
    {
        "#",
        { "#" },
    },
    {
        "*.py",
        { "*.py" },
    },
    {
        "*,-*.py",
        { "*", "-*.py" },
    },
    {
        "#cpp",
        { "*.cpp", "*.cxx", "*.hpp", "*.hxx" },
    },
    {
        "-#cpp",
        { "-*.cpp", "-*.cxx", "-*.hpp", "-*.hxx" },
    },
    {
        "#c,#cpp",
        { "*.c", "*.h", "*.cpp", "*.cxx", "*.hpp", "*.hxx" },
    },
    {
        "#cpp,-*.cpp",
        { "*.cpp", "*.cxx", "*.hpp", "*.hxx", "-*.cpp" },
    },
    {
        "#cpp,-#hpp",
        { "*.cpp", "*.cxx", "*.hpp", "*.hxx", "-*.hpp", "-*.hxx" },
    },
    {
        "\\#comment",
        { "\\#comment" },
    },
    {
        "#unknown",
        { "#unknown" },
    }
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        typetag_test, typetag_test, values_in<typetag_param>(typetag_cases)
);

// Cases use `split` which removes empty ones, still a risk in vector overload
TEST_F(typetag_test, empty_glob) {
    using v = std::vector<std::string_view>;
    EXPECT_EQ(typetag_database{}.expand(v{ "" }), v{ "" });
}

}  // namespace glug::glob::unit_test

