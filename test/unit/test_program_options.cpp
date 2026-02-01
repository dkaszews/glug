// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/program_options.hpp"

#include "parametrized.hpp"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glug::program::unit_test {

struct program_options_param {
    std::vector<std::string_view> args{};
    program_options expected{};

    friend std::ostream&
    operator<<(std::ostream& os, const program_options_param& param) {
        std::ignore = param;
        return os;
    }
};

class program_options_test :
    public testing::TestWithParam<program_options_param> {};

// NOLINTNEXTLINE
TEST_P(program_options_test, test) {
    const auto& [args, expected] = GetParam();
    EXPECT_EQ(program_options::parse(args), expected);
}

static const auto program_options_cases = std::vector<program_options_param>{
    {
        { "glug", "pattern", "file" },
        {
            .patterns = { "pattern" },
            .paths = { "file" },
        },
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        test, program_options_test, testing::ValuesIn(program_options_cases)
);

}  // namespace glug::program::unit_test

