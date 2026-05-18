// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include "glug/glob.hpp"
#include "glug/regex.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

/**
 * Filtering options applicable to searching files.
 */
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

/**
 * Converts enum to string.
 *
 * @param os Output stream.
 * @param value Enum value.
 * @return Stringified enum value.
 */
std::ostream& operator<<(std::ostream& os, decision value) noexcept;

/**
 * Represents a list of decomposed globs used as an ignore filter.
 *
 * The source's parent is used as a base for anchored filters.
 */
class ignore {
    public:
    /**
     * Default-constructs empty filter, always returning `decision::undecided`.
     */
    ignore() noexcept = default;

    /**
     * Constructs filter from sequence of globs.
     *
     * The anchor is left as empty, which makes any anchored patterns behave as
     * absolute paths, from the root of the filesystem.
     *
     * @param globs Sequence of globs.
     */
    explicit ignore(std::span<const glob::decomposition> globs) :
        ignore{ globs, "" } {}

    /**
     * @copydoc ignore(std::span<const glob::decomposition> globs)
     */
    explicit ignore(std::span<const std::string_view> globs) :
        ignore{ globs, "" } {}

    /**
     * Constructs filter from sequence of globs.
     *
     * @param globs Sequence of globs.
     * @param anchor Relative point for anchored globs.
     */
    ignore(std::span<const glob::decomposition> globs,
           const std::filesystem::path& anchor);

    /**
     * @copydoc ignore(std::span<const glob::decomposition>, const std::filesystem::path&)
     */
    ignore(std::span<const std::string_view> globs,
           const std::filesystem::path& anchor);

    /**
     * Check an entry against the list of globs.
     *
     * @param entry File or directory.
     * @return `decision::excluded` if last matching glob is not inverted,
     * `decision::included` if it is, `decision::undecided` if none match.
     * @see `decision`
     */
    [[nodiscard]] decision
    apply(const std::filesystem::directory_entry& entry) const noexcept;

    /**
     * @copydoc apply()
     */
    [[nodiscard]] decision
    operator()(const std::filesystem::directory_entry& entry) const noexcept {
        return apply(entry);
    }

    private:
    struct ignore_item {
        bool is_inverted{};
        bool is_anchored{};
        bool is_directory{};
        regex::engine regex{};
    };

    std::vector<ignore_item> items{};
};

/**
 * Allows for additional filtering of files and directories.
 *
 * @see glug::glob::decompose_mode::select
 */
class select {
    public:
    /**
     * Default-constructs empty filter, always returning `decision::undecided`.
     */
    select() noexcept = default;

    /**
     * Constructs filter from sequence of globs.
     *
     * The anchor is left as empty, which makes any anchored patterns behave as
     * absolute paths, from the root of the filesystem.
     *
     * @param globs Sequence of globs.
     */
    explicit select(std::span<const glob::decomposition> globs) :
        select{ globs, "" } {}

    /**
     * @copydoc select(std::span<const glob::decomposition> globs)
     */
    explicit select(std::span<const std::string_view> globs) :
        select{ globs, "" } {}

    /**
     * @copydoc select(std::span<const glob::decomposition> globs)
     *
     * Treats globs as comma-separated list.
     *
     * @see glug::glob::split()
     */
    explicit select(std::string_view globs) :
        select{ globs, "" } {}

    /**
     * Constructs filter from sequence of globs.
     *
     * @param globs Sequence of globs.
     * @param anchor Relative point for anchored globs.
     */
    select(std::span<const glob::decomposition> globs,
           const std::filesystem::path& anchor);

    /**
     * @copydoc select(std::span<const glob::decomposition> globs, const std::filesystem::path&)
     */
    select(std::span<const std::string_view> globs,
           const std::filesystem::path& anchor);

    /**
     * @copydoc select(std::span<const glob::decomposition> globs, const std::filesystem::path&)
     *
     * Treats globs as comma-separated list.
     *
     * @see glug::glob::split()
     */
    select(std::string_view globs, const std::filesystem::path& anchor);

    /**
     * Check a file or directory against the list of globs.
     *
     * Files and directories are treated as separate types, with no overlap.
     *
     * @param entry File or directory.
     * @return `decision::excluded` if last matching glob is not inverted,
     * `decision::included` if it is. If none match, but at least one positive
     * glob exists, returns `decision::excluded`, else `decision::undecided`.
     * @see `decision`
     */
    [[nodiscard]] decision
    apply(const std::filesystem::directory_entry& entry) const noexcept;

    /**
     * @copydoc apply
     */
    [[nodiscard]] decision
    operator()(const std::filesystem::directory_entry& entry) const noexcept {
        return apply(entry);
    }

    private:
    struct ignore_item {
        bool is_inverted{};
        bool is_anchored{};
        regex::engine regex{};
    };

    std::vector<ignore_item> files{};
    std::vector<ignore_item> dirs{};
    decision files_fallback{};
    decision dirs_fallback{};
};

}  // namespace glug::filter

