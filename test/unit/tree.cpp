// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "tree.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace glug::unit_test {

namespace {

std::filesystem::path make_temp_directory() {
    auto i = size_t{ 0 };
    auto result = std::filesystem::path{};
    bool created = false;
    while (!created) {
        result = std::filesystem::temp_directory_path()
                / ("glug_test." + std::to_string(i++));
        created = std::filesystem::create_directory(result);
    };
    return result;
}

}  // namespace

temp_fs::temp_fs() :
    path_value{ make_temp_directory() } {}

temp_fs::~temp_fs() { std::filesystem::remove_all(path()); }

node file::leaf() const { return *this; }

void file::move(const std::filesystem::path& destination) {
    path_value = destination / name();
}

void file::materialize(const temp_fs& temp) const {
    std::ofstream{ temp / path() } << contents();
}

namespace {

std::string escape_literal(std::string_view s) {
    static constexpr auto escapes
            = std::string_view{ "\'\"\?\\\a\b\f\n\r\t\v" };
    auto result = std::string{};
    for (const auto& c : s) {
        if (escapes.find(c) != escapes.npos) {
            result += '\\';
            break;
        }
        result += c;
    }
    return result;
}

}  // namespace

std::ostream& operator<<(std::ostream& os, const file& file) {
    os << "file{ " << file.name();
    if (!file.contents().empty()) {
        os << ", \"" << escape_literal(file.contents()) << "\"";
    }
    return os << " }";
}

link::link(std::string_view name, std::filesystem::path target) :
    path_value{ name },
    target_value{ std::move(target) } {}

node link::leaf() const { return *this; }

void link::move(const std::filesystem::path& destination) {
    path_value = destination / name();
}

void link::materialize(const temp_fs& temp) const {
    std::filesystem::create_symlink(target(), temp / path());
}

std::ostream& operator<<(std::ostream& os, const link& link) {
    return os << "link{ " << link.path() << ", " << link.target() << " }";
}

dir::dir(std::string_view name, const std::vector<node>& contents) :
    path_value{ name },
    contents_value{ contents } {

    for (auto& child : contents_value) {
        child.move(path());
    }
}

node dir::leaf() const {
    return !contents().empty() ? contents().front().leaf() : node{ *this };
}

void dir::move(const std::filesystem::path& destination) {
    path_value = destination / name();
    for (auto& child : contents_value) {
        child.move(path());
    }
}

void dir::materialize(const temp_fs& temp) const {
    std::filesystem::create_directory(temp / path());
    for (const auto& child : contents()) {
        child.materialize(temp);
    }
}

std::ostream& operator<<(std::ostream& os, const dir& dir) {
    os << "dir{ " << dir.name();
    if (dir.contents().empty()) {
        return os << " }";
    }

    os << ", { ";
    for (const auto& child : dir.contents()) {
        os << child << ", ";
    }
    return os << " } }";
}

const std::filesystem::path& node::path() const {
    const auto visitor = [](auto& node) -> auto& { return node.path(); };
    return std::visit(visitor, value);
}

std::filesystem::path node::name() const {
    const auto visitor = [](auto& node) { return node.name(); };
    return std::visit(visitor, value);
}

node node::leaf() const {
    const auto visitor = [](auto& node) { return node.leaf(); };
    return std::visit(visitor, value);
}

void node::move(const std::filesystem::path& destination) {
    const auto visitor = [&destination](auto& node) { node.move(destination); };
    std::visit(visitor, value);
}

void node::materialize(const temp_fs& temp) const {
    const auto visitor = [&temp](auto& node) { node.materialize(temp); };
    std::visit(visitor, value);
}

bool operator==(const node& lhs, const node& rhs) {
    return lhs.value == rhs.value;
}

std::ostream& operator<<(std::ostream& os, const node& node) {
    const auto visitor = [&os](auto& node) -> auto& { return os << node; };
    return std::visit(visitor, node.value);
}

}  // namespace glug::unit_test

