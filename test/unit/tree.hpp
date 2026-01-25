// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
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

    [[nodiscard]] const auto& path() const { return path_value; }
    // NOLINTNEXTLINE(google-explicit-constructor): Desired implicit
    [[nodiscard]] operator const std::filesystem::path&() const {
        return path();
    }

    private:
    std::filesystem::path path_value{};
};

class node;

class file {
    public:
    explicit file(std::string_view name) :
        file{ name, {} } {}
    // No good alternative, `path` would imply multi-element support
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    file(std::string_view name, std::string_view contents) :
        path_value{ name },
        contents_value{ contents } {}

    [[nodiscard]] const auto& path() const { return path_value; }
    [[nodiscard]] auto name() const { return path().filename(); }
    [[nodiscard]] const auto& contents() const { return contents_value; }
    [[nodiscard]] node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp) const;

    bool operator==(const file&) const = default;

    private:
    std::filesystem::path path_value{};
    std::string contents_value{};
};
inline file operator""_f(const char* name, [[maybe_unused]] size_t n) {
    return file{ name };
}
std::ostream& operator<<(std::ostream& os, const file& file);

class link {
    public:
    link(std::string_view name, std::filesystem::path target);

    [[nodiscard]] const auto& path() const { return path_value; }
    [[nodiscard]] auto name() const { return path_value.filename(); }
    [[nodiscard]] const auto& target() const { return target_value; }
    [[nodiscard]] node leaf() const;

    void move(const std::filesystem::path& destination);
    // Might throw `std::filesystem::filesystem_error` on Windows
    void materialize(const temp_fs& temp) const;

    bool operator==(const link&) const = default;

    private:
    std::filesystem::path path_value{};
    std::filesystem::path target_value{};
};
std::ostream& operator<<(std::ostream& os, const link& link);

class dir {
    public:
    explicit dir(std::string_view name);
    dir(std::string_view name, const std::vector<node>& contents);

    [[nodiscard]] const auto& path() const { return path_value; }
    [[nodiscard]] auto name() const { return path_value.filename(); }
    [[nodiscard]] const auto& contents() const { return contents_value; }
    [[nodiscard]] node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp) const;

    bool operator==(const dir& other) const;

    private:
    std::filesystem::path path_value{};
    // Vector of incomplete types requires all definitions out-of-line.
    std::vector<node> contents_value;
};
dir operator""_d(const char* name, [[maybe_unused]] size_t n);
std::ostream& operator<<(std::ostream& os, const dir& dir);

class node {
    public:
    // NOLINTNEXTLINE(google-explicit-constructor): Desired implicit
    node(const file& file) :
        value{ file } {}

    // NOLINTNEXTLINE(google-explicit-constructor): Desired implicit
    node(const dir& dir) :
        value{ dir } {}

    // NOLINTNEXTLINE(google-explicit-constructor): Desired implicit
    node(const link& link) :
        value{ link } {}

    [[nodiscard]] const std::filesystem::path& path() const;
    [[nodiscard]] std::filesystem::path name() const;
    [[nodiscard]] node leaf() const;

    void move(const std::filesystem::path& destination);
    void materialize(const temp_fs& temp) const;

    bool operator==(const node&) const = default;
    friend std::ostream& operator<<(std::ostream& os, const node& node);

    private:
    std::variant<file, dir, link> value;
};

// Warning: shorthand only works for single level, needs right-parenthesising
// after that to ensure proper precedence. Fixing would require additional API
// with `leaf()` returning a modifiable reference to add `node` to.
inline dir operator/(const dir& dir, const node& node) {
    return { dir.path().string(), { node } };
}

}  // namespace glug::unit_test

