#pragma once

#include "glug/detail/mockable/directory_entry.hpp"
#include "glug/glob.hpp"

#include <filesystem>
#include <iostream>
#include <regex>
#include <vector>

namespace glug::ignore {

/**
 * Represents a filter decision about an entry (file or directory).
 */
enum class decision {
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
    ignored,
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
class filter {
    public:
    filter() noexcept = default;

    filter(const std::vector<glob::decomposition>& globs,
           const std::filesystem::path& source = "") noexcept;

    filter(const std::vector<std::string_view>& globs,
           const std::filesystem::path& source = "") noexcept;

    /**
     * Check a file or directory against the list of globs.
     * @see decision
     */
    decision is_ignored(
            const mockable<std::filesystem::directory_entry>& entry
    ) const noexcept;

    private:
    struct ignore_item {
        bool is_negative{};
        bool is_anchored{};
        bool is_directory{};
        std::regex regex{};
    };

    std::vector<ignore_item> items{};
};

}  // namespace glug::ignore

