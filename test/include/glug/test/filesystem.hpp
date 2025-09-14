#pragma once

#include "glug/detail/mockable/access.hpp"
#include "glug/detail/mockable/directory_entry.hpp"

#include <deque>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace glug::unit_test {

// Use mocks instead of tmpdir to assert which files are listed and read.
// TODO: Rerun with actual tmpdir to verify more real usage.
struct test_fs {
    using directory_entry = mockable<std::filesystem::directory_entry>;
    using files = std::vector<test_fs>;
    using lines = std::vector<std::string>;

    test_fs(const directory_entry& entry) :
        entry{ entry } {};

    void create(access_mock& mock, std::filesystem::path prefix = "") const;
    std::deque<directory_entry> unwind(std::filesystem::path prefix = "") const;

    directory_entry entry;
    std::variant<std::nullopt_t, files, lines> contents{ std::nullopt };
};

test_fs operator/(const test_fs& tree, const test_fs::files& contents);
test_fs operator/(const test_fs& tree, const test_fs::lines& contents);

}  // namespace glug::unit_test

