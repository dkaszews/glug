// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

#include "glug/glob.hpp"
#include "glug/regex.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace glug::filter {

/**
 * Represents a filter decision about an entry (file or directory).
 */
enum class decision : uint8_t {
    /**
     * Filter does not consider the entry.
     *
     * Filters in parent directories should be checked recursively.
     * If no such filters exists, the file is not ignored.
     */
    undecided,
    /**
     * Filter ignores the entry.
     */
    excluded,
    /**
     * Filter explicitly includes the entry.
     *
     * Filters in parent directories are not to be checked.
     */
    included,
};

std::ostream& operator<<(std::ostream& os, decision value) noexcept;

/**
 * Represents a list of decomposed globs used as an ignore filter.
 *
 * The source's parent is used as a base for anchored filters.
 */
class ignore {
    public:
    ignore() noexcept = default;

    // TODO(#70): Replace `source` with `anchor`
    explicit ignore(const std::vector<glob::decomposition>& globs) :
        ignore{ globs, "" } {}

    explicit ignore(const std::vector<std::string_view>& globs) noexcept :
        ignore{ globs, "" } {}

    ignore(const std::vector<glob::decomposition>& globs,
           const std::filesystem::path& source);

    ignore(const std::vector<std::string_view>& globs,
           const std::filesystem::path& source);

    /**
     * Check a file or directory against the list of globs.
     * @see decision
     */
    [[nodiscard]] decision
    is_ignored(const std::filesystem::directory_entry& entry) const noexcept;

    private:
    struct ignore_item {
        bool is_negative{};
        bool is_anchored{};
        bool is_directory{};
        regex::engine regex{};
    };

    std::vector<ignore_item> items{};
};

}  // namespace glug::filter

