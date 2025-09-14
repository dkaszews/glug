#pragma once

#include "glug/detail/mockable/directory_entry.hpp"

#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace glug::unit_test {

std::filesystem::path create_temp_dir();

// TODO: Rename to tree
struct tree {
    using files = std::vector<tree>;
    using lines = std::vector<std::string>;

    tree(const char* path) :
        path{ path } {}

    void create(std::filesystem::path prefix = "") const;
    std::deque<mockable<std::filesystem::directory_entry>>
    unwind(std::filesystem::path prefix = "") const;

    std::filesystem::path path{};
    std::variant<std::nullopt_t, files, lines> contents{ std::nullopt };
};

tree operator&(const tree& node, const tree::files& contents);
tree operator&(const tree& node, const tree::lines& contents);

}  // namespace glug::unit_test

