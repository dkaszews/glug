// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/program_options.hpp"

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glug::program {

// NOLINTNEXTLINE(misc-use-internal-linkage): Would break ADL
std::ostream& operator<<(std::ostream& os, const program_options& options) {
    os << "{ ";
    const auto stream_vector = [&](const auto& v, const std::string& name) {
        if (!v.empty()) {
            os << "." << name << " = " << testing::PrintToString(v) << ", ";
        }
    };
    const auto stream_optional = [&](const auto& o, const std::string& name) {
        if (o.has_value()) {
            os << "." << name << " = " << *o << ", ";
        }
    };
    const auto stream_flag = [&](bool flag, const std::string& name) {
        if (flag) {
            os << "." << name << " = true, ";
        }
    };

    stream_vector(options.patterns, "patterns");
    stream_vector(options.paths, "paths");
    stream_optional(options.filters, "filters");
    stream_flag(options.list, "list");
    stream_flag(options.help.show_help, "help.show_help");
    stream_flag(options.help.show_tags, "help.show_tags");
    stream_flag(options.help.show_version, "help.show_version");
    stream_flag(options.help.show_license, "help.show_license");

    os << "}";
    return os;
}

}  // namespace glug::program

namespace glug::program::unit_test {

struct program_options_param {
    std::vector<std::string_view> args{};
    program_options expected{};
    std::optional<parse_error> error{};

    friend std::ostream&
    operator<<(std::ostream& os, const program_options_param& param) {
        return os << testing::PrintToString(param.args);
    }
};

class program_options_test :
    public testing::TestWithParam<program_options_param> {};

// NOLINTNEXTLINE
TEST_P(program_options_test, test) {
    const auto& [args, expected, error] = GetParam();
    auto actual = program_options{};
    const auto parse = [&]() { actual = program_options::parse(args); };

    if (error.has_value()) {
        EXPECT_THAT(parse, testing::ThrowsMessage<parse_error>(error->what()));
        return;
    }

    // NOLINTNEXTLINE
    EXPECT_NO_THROW(parse());
    EXPECT_EQ(actual, expected);
}

static const auto program_options_cases = std::vector<program_options_param>{
    {
        {},
        {},
        parse_error{
            "Exactly 1 option from [PATTERN,--regexp,--no-regexp] is "
            "required",
        },
    },
    {
        { "a" },
        { .patterns = { "a" } },
    },
    {
        { "a", "b" },
        { .patterns = { "a" }, .paths = { "b" } },
    },
    {
        { "a", "b", "c" },
        { .patterns = { "a" }, .paths = { "b", "c" } },
    },
    {
        { "a", "b", "c", "d" },
        { .patterns = { "a" }, .paths = { "b", "c", "d" } },
    },
    {
        { "-e", "x" },
        { .patterns = { "x" } },
    },
    {
        { "-e", "x", "-e", "y" },
        { .patterns = { "x", "y" } },
    },
    {
        { "-e", "x", "-e", "y", "-e", "z" },
        { .patterns = { "x", "y", "z" } },
    },
    {
        { "-e", "x", "-e", "y", "-e", "z", "a" },
        { .patterns = { "x", "y", "z" }, .paths = { "a" } },
    },
    {
        { "-e", "x", "-e", "y", "-e", "z", "a", "b" },
        { .patterns = { "x", "y", "z" }, .paths = { "a", "b" } },
    },
    {
        { "-e", "x", "-e", "y", "-e", "z", "a", "b", "c" },
        { .patterns = { "x", "y", "z" }, .paths = { "a", "b", "c" } },
    },
    {
        { "-E" },
        { .list = true },
    },
    {
        { "-E", "a", "b", "c" },
        { .paths = { "a", "b", "c" }, .list = true },
    },
    {
        { "-Eex" },
        {},
        parse_error{ "--regexp excludes --no-regexp" },
    },
    {
        { "-f", "#cpp", "a", "b", "c" },
        { .patterns = { "a" }, .paths = { "b", "c" }, .filters = { "#cpp" } },
    },
    {
        { "-f" },
        {},
        parse_error{ "--filter: 1 required TEXT missing" },
    },
    {
        { "--help" },
        { .help = { .show_help = true } },
    },
    {
        { "--help-tags" },
        { .help = { .show_tags = true } },
    },
    {
        { "--version" },
        { .help = { .show_version = true } },
    },
    {
        { "--license" },
        { .help = { .show_license = true } },
    },
    {
        { "--invalid" },
        {},
        parse_error{ "The following argument was not expected: --invalid" },
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        test, program_options_test, testing::ValuesIn(program_options_cases)
);

// NOLINTNEXTLINE
TEST_F(program_options_test, help) {
    using testing::HasSubstr;
    const auto help = program_options::get_help();
    EXPECT_THAT(help, HasSubstr("Usage: [OPTIONS] [PATTERN] [PATH...]"));
}

}  // namespace glug::program::unit_test

