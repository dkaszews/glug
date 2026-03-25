// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#pragma once

#include <filesystem>
#include <optional>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace glug::cli {

class cli_error : public std::invalid_argument {
    public:
    using std::invalid_argument::invalid_argument;
};

struct cli_options {
    std::vector<std::string> patterns{};
    std::vector<std::filesystem::path> paths{};
    std::optional<std::string> filters{};

    bool list{};

    struct help_flags {
        // GCOVR_EXCL_START - Ubuntu bug, doubled initializers
        bool show_help{};
        bool show_version{};
        bool show_license{};
        bool show_tags{};
        // GCOVR_EXCL_STOP

        explicit operator bool() const { return *this != decltype(*this){}; }

        // GCOVR_EXCL_START
        bool operator==(const help_flags&) const noexcept = default;
        // GCOVR_EXCL_STOP
    } help{};

    static cli_options parse(std::span<const std::string_view> args);
    static std::string get_help();

    // GCOVR_EXCL_START
    bool operator==(const cli_options&) const noexcept = default;
    // GCOVR_EXCL_STOP
};

}  // namespace glug::cli

