// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include "glug/filter.hpp"

#include <cstddef>
#include <deque>
#include <filesystem>
#include <iterator>
#include <vector>

namespace glug::filesystem {

/**
 * Provides additional options to explorer.
 */
struct explorer_options {
    /**
     * Extra filters to specify files and/or directories should be returned.
     *
     * @see glug::filter::select
     */
    filter::select select{};
};

/**
 * Recursively lists directory contents, respecting .gitignore rules.
 *
 * Interface roughly matches `std::filesystem::directory_iterator`
 * and the results should be exactly the same as `git ls-files` command.
 */
class explorer {
    struct level;

    public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::filesystem::directory_entry;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = std::input_iterator_tag;

    explorer() noexcept = default;
    explicit explorer(const std::filesystem::path& root) :
        explorer(root, {}) {}

    explorer(
            const std::filesystem::path& root, const explorer_options& options
    );

    [[nodiscard]] explorer begin() const noexcept { return *this; }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static): interface
    [[nodiscard]] explorer end() const noexcept { return {}; }

    [[nodiscard]] reference operator*() const;
    [[nodiscard]] pointer operator->() const;
    explorer& operator++();
    explorer operator++(int);

    friend bool operator==(const explorer& lhs, const explorer& rhs) noexcept;
    friend bool operator!=(const explorer& lhs, const explorer& rhs) noexcept;

    private:
    friend class explorer_impl;

    struct level {
        filter::ignore filter{};
        // PERF: Try using just iterator, without sorting whole directory
        std::deque<std::filesystem::directory_entry> entries{};
        bool is_root{};

        friend bool operator==(const level& lhs, const level& rhs) {
            // Don't compare filters as they are transient cache.
            // Identical entries must have encountered the same filters,
            // which could be found again from a set of entry parents.
            // `is_root` is also ignored as more of a filter property.
            return lhs.entries == rhs.entries;
        }
    };

    std::vector<level> stack{};
    explorer_options options{};
};

}  // namespace glug::filesystem

