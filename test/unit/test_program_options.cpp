// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/program_options.hpp"

#include <format>
#include <ostream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glug::program {

namespace {

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& container) {
    bool first = true;
    os << "[ ";
    for (const auto& value : container) {
        if (!std::exchange(first, false)) {
            os << ", ";
        }
        os << value;
    }
    return os << " ]";
}

}  // namespace

std::ostream& operator<<(std::ostream& os, const program_options& options) {
    os << "{ ";
    const auto stream_vector = [&](const auto& v, const std::string& name) {
        if (!v.empty()) {
            os << "." << name << " = " << v << ", ";
        }
    };
    const auto stream_flag = [&](bool flag, const std::string& name) {
        if (flag) {
            os << "." << name << " = true, ";
        }
    };

    stream_vector(options.patterns, "patterns");
    stream_vector(options.paths, "paths");
    stream_vector(options.filters, "filters");
    stream_flag(options.list, "list");
    stream_flag(options.help, "help");
    stream_flag(options.version, "version");
    stream_flag(options.license, "license");

    os << "}";
    return os;
}

}  // namespace glug::program

namespace glug::program::unit_test {

struct no_error {};

struct program_options_param {
    std::vector<std::string_view> args{};
    // Putting into vector would require positive cases to spell out typename
    program_options expected{};
    std::variant<no_error, require_error, exclude_error> error{};

    friend std::ostream&
    operator<<(std::ostream& os, const program_options_param& param) {
        os << param.args;
        return os;
    }
};

class program_options_test :
    public testing::TestWithParam<program_options_param> {};

// NOLINTNEXTLINE
TEST_P(program_options_test, test) {
    const auto& [args, expected, error] = GetParam();
    auto actual = program_options{};
    const auto parse = [&]() { actual = program_options::parse(args); };

    // NOLINTNEXTLINE: Some FP about null vtable in all the way in `std::visit`
    if (error.index() == 0) {
        // NOLINTNEXTLINE
        EXPECT_NO_THROW(parse());
        EXPECT_EQ(actual, expected);
        return;
    }

    const auto error_visitor = [&]<typename E>(const E& e) {
        if constexpr (!std::is_same_v<E, no_error>) {
            // NOLINTNEXTLINE
            EXPECT_THAT(parse, testing::ThrowsMessage<E>(e.what()));
        }
    };
    std::visit(error_visitor, error);
}

static const auto program_options_cases = std::vector<program_options_param>{
    {
        {},
        {},
        require_error{
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
        exclude_error{ "--regexp excludes --no-regexp" },
    },
    {
        { "-f", "#cpp", "a", "b", "c" },
        { .patterns = { "a" }, .paths = { "b", "c" }, .filters = { "#cpp" } },
    },
    {
        { "--help" },
        { .help = true },
    },
    {
        { "--version" },
        { .version = true },
    },
    {
        { "--license" },
        { .license = true },
    },
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(
        test, program_options_test, testing::ValuesIn(program_options_cases)
);

// TODO: Remove or replace with a golden standard
// NOLINTNEXTLINE
TEST_F(program_options_test, help) { std::cout << program_options::get_help(); }

}  // namespace glug::program::unit_test

