// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace glug::unit_test {

class temp_fs {
    public:
    temp_fs();
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

class node;

class file {
    public:
    explicit file(std::string_view name) :
        file{ name, {} } {}
    // No good alternative, `path` would imply multi-element support
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    file(std::string_view name, std::string_view contents) :
        path_{ name },
        contents_{ contents } {}

    const auto& path() const { return path_; }
    auto name() const { return path_.filename(); }
    const auto& contents() const { return contents_; }
    node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp) const;

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

class link {
    public:
    link(std::string_view name, const std::filesystem::path& target);

    const auto& path() const { return path_; }
    auto name() const { return path_.filename(); }
    const auto& target() const { return target_; }
    node leaf() const;

    void move(const std::filesystem::path& destination);
    // Might throw `std::filesystem::filesystem_error` on Windows
    void materialize(const temp_fs& temp) const;

    private:
    std::filesystem::path path_{};
    std::filesystem::path target_{};
};
inline bool operator==(const link& lhs, const link& rhs) {
    return std::tie(lhs.path(), lhs.target())
            == std::tie(rhs.path(), rhs.target());
}
std::ostream& operator<<(std::ostream& os, const link& link);

class dir {
    public:
    explicit dir(std::string_view name) :
        dir{ name, {} } {}
    dir(std::string_view name, const std::vector<node>& contents);

    const auto& path() const { return path_; }
    auto name() const { return path_.filename(); }
    const auto& contents() const { return contents_; }
    node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp) const;

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
    node(const link& link) :
        variant_{ link } {}

    const std::filesystem::path& path() const;
    std::filesystem::path name() const;
    node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp) const;

    friend bool operator==(const node& lhs, const node& rhs);
    friend std::ostream& operator<<(std::ostream& os, const node& node);

    private:
    std::variant<file, dir, link> variant_;
};

// Warning: shorthand only works for single level, needs right-parenthesising
// after that to ensure proper precedence. Fixing would require additional API
// with `leaf()` returning a modifiable reference to add `node` to.
inline dir operator/(const dir& dir, const node& node) {
    return { dir.path().string(), { node } };
}

}  // namespace glug::unit_test

