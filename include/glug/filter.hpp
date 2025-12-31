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

    explicit ignore(const std::vector<glob::decomposition>& globs) :
        ignore{ globs, "" } {}

    explicit ignore(const std::vector<std::string_view>& globs) noexcept :
        ignore{ globs, "" } {}

    ignore(const std::vector<glob::decomposition>& globs,
           const std::filesystem::path& anchor);

    ignore(const std::vector<std::string_view>& globs,
           const std::filesystem::path& anchor);

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

/**
 * Allows for additional filtering of files and directories.
 *
 * @see glug::glob::decompose_mode::select_mode
 */
class select {
    public:
    select() noexcept = default;

    explicit select(const std::vector<glob::decomposition>& globs) :
        select{ globs, "" } {}

    explicit select(const std::vector<std::string_view>& globs) :
        select{ globs, "" } {}

    explicit select(std::string_view globs) :
        select{ globs, "" } {}

    select(const std::vector<glob::decomposition>& globs,
           const std::filesystem::path& anchor);

    select(const std::vector<std::string_view>& globs,
           const std::filesystem::path& anchor);

    select(std::string_view globs, const std::filesystem::path& anchor);

    /**
     * Check a file or directory against the list of globs.
     *
     * Files and directories are treated as separate types, with no overlap.
     *
     * If last matching glob is negative, returns `decision::ignored`.
     * Else, if last matching glob is positive, returns `decision::included`.
     * Else, if at least one positive glob exists, returns `decision::ignored`.
     * Else, returns `decision::undecided`.
     *
     * @see decision
     */
    [[nodiscard]] decision
    is_ignored(const std::filesystem::directory_entry& entry) const noexcept;

    private:
    struct ignore_item {
        bool is_negative{};
        bool is_anchored{};
        regex::engine regex{};
    };

    std::vector<ignore_item> files{};
    std::vector<ignore_item> dirs{};
    decision files_fallback{};
    decision dirs_fallback{};
};

}  // namespace glug::filter

