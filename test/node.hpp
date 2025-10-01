// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace glug::unit_test {

// TODO: Document
class node {
    public:
    static std::filesystem::path temp_dir();

    node(const char* path);
    node(const char* path, const char* contents);
    node(const char* path, const std::vector<node>& contents);

    std::filesystem::directory_entry create() const;
    void remove() const;

    const std::filesystem::path name() const { return path.filename(); }
    std::vector<std::filesystem::path> tree() const;

    friend bool operator==(const node& lhs, const node& rhs);
    friend bool operator<(const node& lhs, const node& rhs);
    friend std::ostream& operator<<(std::ostream& os, const node& obj);

    private:
    std::filesystem::path path{};
    bool is_dir{};
    std::variant<std::string, std::vector<node>> inner{};
};

inline node operator""_n(const char* path, size_t) { return node{ path }; }

}  // namespace glug::unit_test

