// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#pragma once

#include <filesystem>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace glug::program {

class parse_error : public std::invalid_argument {
    public:
    using std::invalid_argument::invalid_argument;
};

class exclude_error : public parse_error {
    public:
    using parse_error::parse_error;
};

class require_error : public parse_error {
    public:
    using parse_error::parse_error;
};

// TODO: Separate per group?
struct program_flags {
    bool list{};
    bool version{};
    bool license{};
    bool help{};
    bool help_filter{};
    bool help_tags{};

    bool operator==(const program_flags&) const noexcept = default;
};
// TODO: Replace with `std::format`
std::ostream& operator<<(std::ostream& os, const program_flags& flags);

struct program_options {
    std::vector<std::string> patterns{};
    std::vector<std::filesystem::path> paths{};
    std::vector<std::string> filters{};
    program_flags flags{};

    static program_options parse(std::span<const std::string_view> args);
    static std::string get_help();

    bool operator==(const program_options&) const noexcept = default;
};
std::ostream& operator<<(std::ostream& os, const program_options& options);

}  // namespace glug::program

