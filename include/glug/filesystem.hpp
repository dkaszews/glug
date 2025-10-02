#pragma once

#include "glug/filter.hpp"

#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace glug::filesystem {

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
    explorer(const std::filesystem::path& root);

    explorer begin() const noexcept { return *this; }
    explorer end() const noexcept { return {}; }

    reference operator*() const;
    pointer operator->() const;
    explorer& operator++();
    explorer operator++(int);

    friend bool operator==(const explorer& lhs, const explorer& rhs) noexcept;
    friend bool operator!=(const explorer& lhs, const explorer& rhs) noexcept;

    private:
    friend class explorer_impl;

    struct level {
        glob::filter filter{};
        // PERF: Try using just iterator, without sorting whole directory
        std::deque<std::filesystem::directory_entry> entries{};

        friend bool operator==(const level& lhs, const level& rhs) {
            // Don't compare filters as they are transient cache.
            // Identical entries must have encountered the same filters,
            // which could be found again from a set of entry parents.
            return lhs.entries == rhs.entries;
        }
    };

    std::vector<level> stack{};
};

}  // namespace glug::filesystem

