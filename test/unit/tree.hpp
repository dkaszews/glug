// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace glug::unit_test {

namespace old {

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

}  // namespace old

struct file {
    std::string name{};
    std::string content{};
};
inline file operator""_f(const char* name, size_t) { return { name }; }
inline bool operator==(const file& lhs, const file& rhs) {
    return std::tie(lhs.name, lhs.content) == std::tie(rhs.name, rhs.content);
}

struct link {
    std::string name{};
    std::filesystem::path target{};
};
inline link operator""_l(const char* name, size_t) { return { name }; }
inline bool operator==(const link& lhs, const link& rhs) {
    return std::tie(lhs.name, lhs.target) == std::tie(rhs.name, rhs.target);
}

struct dir;
using node = std::variant<file, link, dir>;

struct dir {
    std::string name{};
    std::vector<node> content{};
};
inline dir operator""_d(const char* name, size_t) { return { name }; }
inline bool operator==(const dir& lhs, const dir& rhs) {
    return std::tie(lhs.name, lhs.content) == std::tie(rhs.name, rhs.content);
}

class tree {
    public:
    static std::filesystem::path temp_dir() { return old::tree::temp_dir(); }

    explicit tree(const std::filesystem::path& path);
    // TODO: Consider move-able
    tree(const tree&) = delete;
    tree(tree&&) = delete;
    tree& operator=(const tree&) = delete;
    tree& operator=(tree&&) = delete;
    ~tree();

    friend bool operator==(const tree& lhs, const tree& rhs) {
        return lhs.root == rhs.root;
    }

    private:
    node root{};
};

}  // namespace glug::unit_test

