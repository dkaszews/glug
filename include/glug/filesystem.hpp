// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include "glug/filter.hpp"

#include <cstddef>
#include <deque>
#include <filesystem>
#include <iterator>
#include <vector>

/**
 * Classes related to filesystem, files and directories.
 */
namespace glug::filesystem {

/**
 * Provides additional options to explorer.
 */
struct explorer_options {
    /**
     * Extra filters to specify files and/or directories to be returned.
     *
     * @see glug::filter::select
     */
    filter::select select{};
};

/**
 * Recursively lists directory contents, respecting .gitignore rules.
 *
 * Interface roughly matches `std::filesystem::recursive_directory_iterator`
 * and the results should be exactly the same as `git ls-files` command.
 */
class explorer {
    struct level;

    public:
    /// @nodoc
    using value_type = std::filesystem::directory_entry;
    /// @nodoc
    using difference_type = std::ptrdiff_t;
    /// @nodoc
    using pointer = const value_type*;
    /// @nodoc
    using reference = const value_type&;
    /// @nodoc
    using iterator_category = std::forward_iterator_tag;

    /**
     * Default-construct empty `explorer`, which does not find any files.
     *
     * Equivalent to `end()`.
     *
     * @see end()
     */
    explorer() noexcept = default;

    /**
     * Construct explorer for given filesystem root directory.
     *
     * @param root Directory to begin search in.
     * @throws std::filesystem_error if directory does not exist.
     *
     * @todo UT exception.
     */
    explicit explorer(const std::filesystem::path& root) :
        explorer(root, {}) {}

    /**
     * Construct explorer with additional options.
     *
     * @param root Directory to begin search in.
     * @param options Explorer_options that modify search behavior.
     * @throws std::filesystem_error if directory does not exist.
     *
     * @todo UT exception.
     */
    explorer(
            const std::filesystem::path& root, const explorer_options& options
    );

    /**
     * For-range interface.
     *
     * @return `*this`.
     */
    [[nodiscard]] explorer begin() const noexcept { return *this; }
    /**
     * For-range interface.
     *
     * @return default-constructed `explorer`.
     * @see explorer()
     */
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static): interface
    [[nodiscard]] explorer end() const noexcept { return {}; }

    /**
     * Returns current directory entry.
     *
     * Behavior is undefined if explorer is empty, that is equals to `end()`.
     *
     * @return directory entry, guaranteed to be a file.
     */
    [[nodiscard]] reference operator*() const;

    /// @copydoc operator*()
    [[nodiscard]] pointer operator->() const;

    /**
     * Increment iterator onto the next file.
     *
     * If no more files remain, becomes equal to `end()` and should not be
     * dereferenced.
     *
     * @return `*this`.
     */
    explorer& operator++();

    /**
     * Increment iterator onto the next file.
     *
     * If no more files remain, becomes equal to `end()` and should not be
     * dereferenced.
     *
     * @return copy of old value of `*this`.
     */
    explorer operator++(int);

    /**
     * Checks if two `explorer`s are equivalent.
     *
     * If two instances compare equal, then they should produce identical list
     * of results when iterated until empty.
     *
     * @warning Filters are not compared, as they contain uncomparable regex
     * matchers. Therefore comparing two `explorer`s which were not constructed
     * with the same options can produce false positives, where they compare
     * equal, but later diverge when incremented.
     *
     * @param other Instance to compare against.
     * @return True if equal, false otherwise.
     *
     */
    bool operator==(const explorer& other) const noexcept;

    private:
    friend class explorer_impl;

    struct level {
        filter::ignore filter{};
        // PERF: Try using just iterator, without sorting whole directory
        std::deque<std::filesystem::directory_entry> entries{};
        bool is_root{};

        bool operator==(const level& other) const noexcept;
    };

    std::vector<level> stack{};
    explorer_options options{};
};

}  // namespace glug::filesystem

