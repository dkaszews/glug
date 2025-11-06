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

class node;
class temp_fs;

class file {
    public:
    explicit file(const std::string& name) :
        file{ name, {} } {}
    file(const std::string& name, const std::string& contents) :
        path_{ name },
        contents_{ contents } {}

    const auto& path() const { return path_; }
    auto name() const { return path_.filename(); }
    const auto& contents() const { return contents_; }
    node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp);

    private:
    std::filesystem::path path_{};
    std::string contents_{};
};
inline file operator""_f(const char* name, size_t) { return file{ name }; }
inline bool operator==(const file& lhs, const file& rhs) {
    return std::tie(lhs.path(), lhs.contents())
            == std::tie(rhs.path(), rhs.contents());
}
std::ostream& operator<<(std::ostream& os, const file& file);

class dir {
    public:
    explicit dir(const std::string& name) :
        dir{ name, {} } {}
    dir(const std::string& name, const std::vector<node>& contents);

    const auto& path() const { return path_; }
    auto name() const { return path_.filename(); }
    const auto& contents() const { return contents_; }
    node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp);

    private:
    std::filesystem::path path_{};
    std::vector<node> contents_{};
};
inline dir operator""_d(const char* name, size_t) { return dir{ name }; }
inline bool operator==(const dir& lhs, const dir& rhs) {
    return std::tie(lhs.path(), lhs.contents())
            == std::tie(rhs.path(), rhs.contents());
}
std::ostream& operator<<(std::ostream& os, const dir& dir);

class node {
    public:
    node(const file& file) :
        variant_{ file } {}
    node(const dir& dir) :
        variant_{ dir } {}

    const auto& variant() const { return variant_; }
    const std::filesystem::path& path() const;
    std::filesystem::path name() const;
    node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp);

    private:
    std::variant<file, dir> variant_;
};
inline bool operator==(const node& lhs, const node& rhs) {
    return lhs.variant() == rhs.variant();
}
std::ostream& operator<<(std::ostream& os, const node& node);
inline dir operator/(const dir& dir, const node& node) {
    return { dir.path().string(), { node } };
}

std::filesystem::path leaf(const node& node);

class temp_fs {
    public:
    temp_fs();
    // TODO: Consider moveable
    temp_fs(const temp_fs&) = delete;
    temp_fs(temp_fs&&) = delete;
    temp_fs& operator=(const temp_fs&) = delete;
    temp_fs& operator=(temp_fs&&) = delete;
    ~temp_fs();

    const auto& path() const { return path_; }
    operator const std::filesystem::path&() const { return path(); }

    private:
    std::filesystem::path path_{};
};

}  // namespace glug::unit_test

