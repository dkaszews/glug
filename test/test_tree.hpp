// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace glug::unit_test {

class tree {
    public:
    static std::filesystem::path temp_dir();

    tree(const char* path);
    tree(const char* path, const char* contents);
    tree(const char* path, const std::vector<tree>& contents);

    std::filesystem::directory_entry create() const;
    void remove() const;

    const std::filesystem::path name() const { return path.filename(); }
    std::vector<std::filesystem::path> list() const;

    friend bool operator==(const tree& lhs, const tree& rhs);
    friend bool operator<(const tree& lhs, const tree& rhs);
    friend std::ostream& operator<<(std::ostream& os, const tree& obj);

    private:
    std::filesystem::path path{};
    bool is_dir{};
    std::variant<std::string, std::vector<tree>> inner{};
};

inline tree operator""_t(const char* path, size_t) { return tree{ path }; }

}  // namespace glug::unit_test

