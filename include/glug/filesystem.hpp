#pragma once

#include "glug/ignore.hpp"

#include "glug/detail/mockable_fs.hpp"

#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace glug::filesystem {

// TODO: Document
class access {
    public:
    // TODO: Consider switching to iterator-based functions, to limit heap use
    std::deque<std::filesystem::directory_entry>
    list_directory(const std::filesystem::path& path) const;

    std::vector<std::string>
    read_lines(const std::filesystem::path& path) const;
};

// TODO: Document
class explorer {
    struct level;

    public:
    using difference_type = std::ptrdiff_t;
    using value_type = mockable<std::filesystem::directory_entry>;
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
        ignore::filter filter{};
        // PERF: Try using just iterator, without sorting whole directory
        std::deque<mockable<std::filesystem::directory_entry>> entries{};

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

