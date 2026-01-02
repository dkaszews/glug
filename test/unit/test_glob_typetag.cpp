// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/glob.hpp"

#include "parametrized.hpp"

#include <ostream>
#include <string_view>
#include <tuple>

#include <gtest/gtest.h>

namespace glug::glob::unit_test {

struct typetag_param {
    typetag_database db = {};
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
    const auto& [db, globs, expected] = GetParam();
    EXPECT_EQ(db.expand(globs), expected);
}

const auto typetag_cases = std::vector<typetag_param>{
    typetag_param{
        typetag_database{
            { "cpp", "*.cpp,*.hpp" },
        },
        "#cpp",
        { "*.cpp", "*.hpp" },
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        typetag_test, typetag_test, values_in<typetag_param>(typetag_cases)
);

}  // namespace glug::glob::unit_test

