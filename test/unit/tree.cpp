// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "tree.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>

namespace glug::unit_test {

tree::tree(const char* path) :
    path{ path } {

    if (auto s = this->path.string(); !s.empty() && s.back() == '/') {
        s.pop_back();
        this->path = s;
        is_dir = true;
        inner = std::vector<tree>{};
    }
}

tree::tree(const char* path, const char* contents) :
    path{ path },
    is_dir{ false },
    inner{ contents } {}

tree::tree(const char* path, const std::vector<tree>& children) :
    path{ path },
    is_dir{ true },
    inner{ children } {

    const auto add_prefix = [](auto& f, const auto& s, auto& t) -> void {
        if (t.is_dir) {
            for (auto& child : std::get<std::vector<tree>>(t.inner)) {
                child.path = s / child.path;
                f(f, s, child);
            }
        }
    };
    add_prefix(add_prefix, path, *this);
}

std::filesystem::path tree::temp_dir() {
    static auto path = []() {
        auto i = size_t{ 0 };
        auto result = std::filesystem::path{};
        do {
            result = std::filesystem::temp_directory_path()
                    / ("glug_test." + std::to_string(i++));
        } while (!std::filesystem::create_directory(result));
        return result;
    }();
    return path;
}

std::filesystem::directory_entry tree::create() const {
    const auto absolute = temp_dir() / path;
    if (is_dir) {
        std::filesystem::create_directories(absolute);
        for (const auto& child : std::get<std::vector<tree>>(inner)) {
            child.create();
        }
    } else {
        std::filesystem::create_directories(absolute.parent_path());
        std::ofstream{ absolute } << std::get<std::string>(inner);
    }
    return std::filesystem::directory_entry{ absolute };
}

void tree::remove() const { std::filesystem::remove_all(temp_dir() / path); }

std::vector<std::filesystem::path> tree::list() const {
    std::vector<std::filesystem::path> result{};
    const auto fill = [&result](auto& f, const auto& t) -> void {
        if (!t.is_dir) {
            result.push_back(temp_dir() / t.path);
        } else {
            for (const auto& child : std::get<std::vector<tree>>(t.inner)) {
                f(f, child);
            }
        }
    };
    fill(fill, *this);
    return result;
}

bool operator==(const tree& lhs, const tree& rhs) {
    return std::tie(lhs.path, lhs.is_dir) == std::tie(rhs.path, rhs.is_dir);
}

bool operator<(const tree& lhs, const tree& rhs) {
    return std::tie(lhs.path, lhs.is_dir) < std::tie(rhs.path, rhs.is_dir);
}

std::ostream& operator<<(std::ostream& os, const tree& obj) {
    return os << '"' << obj.path.string() << (obj.is_dir ? "/" : "");
}

}  // namespace glug::unit_test

