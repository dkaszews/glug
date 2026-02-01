// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#pragma once

#include <filesystem>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace glug::program {

struct program_flags {
    bool help{};
    bool help_tags{};
    bool version{};
    bool license{};

    bool operator==(const program_flags&) const noexcept = default;
};
// TODO: Replace with `std::format`
std::ostream& operator<<(std::ostream& os, const program_flags& flags);

struct program_options {
    std::vector<std::string> patterns{};
    std::vector<std::filesystem::path> paths{};
    program_flags flags{};

    static program_options parse(std::span<const std::string_view> args);

    bool operator==(const program_options&) const noexcept = default;
};
std::ostream& operator<<(std::ostream& os, const program_options& options);

}  // namespace glug::program

