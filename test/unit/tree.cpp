// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "tree.hpp"

#include <fstream>
#include <string_view>

namespace glug::unit_test {

static std::filesystem::path make_temp_directory() {
    auto i = size_t{ 0 };
    auto path = std::filesystem::path{};
    do {
        path = std::filesystem::temp_directory_path()
                / ("glug_test." + std::to_string(i++));
    } while (!std::filesystem::create_directory(path));
    return path;
}

temp_fs::temp_fs() :
    path_{ make_temp_directory() } {}

temp_fs::~temp_fs() { std::filesystem::remove_all(path()); }

node file::leaf() const { return *this; }

void file::move(const std::filesystem::path& destination) {
    path_ = destination / name();
}

void file::materialize(const temp_fs& temp) const {
    std::ofstream{ temp / path() } << contents();
}

static std::string escape_literal(const std::string& s) {
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
    return s;
}

std::ostream& operator<<(std::ostream& os, const file& file) {
    os << "file{ " << file.name();
    if (!file.contents().empty()) {
        os << ", \"" << escape_literal(file.contents()) << "\"";
    }
    return os << " }";
}

link::link(const std::string& name, const std::filesystem::path& target) :
    path_{ name },
    target_{ target } {}

node link::leaf() const { return *this; }

void link::move(const std::filesystem::path& destination) {
    path_ = destination / name();
}

void link::materialize(const temp_fs& temp) const {
    std::filesystem::create_symlink(target(), temp / path());
}

std::ostream& operator<<(std::ostream& os, const link& link) {
    return os << "link{ " << link.path() << ", " << link.target() << " }";
}

dir::dir(const std::string& name, const std::vector<node>& contents) :
    path_{ name },
    contents_{ contents } {

    for (auto& child : contents_) {
        child.move(path());
    }
}

node dir::leaf() const {
    return !contents().empty() ? contents().front().leaf() : node{ *this };
}

void dir::move(const std::filesystem::path& destination) {
    path_ = destination / name();
    for (auto& child : contents_) {
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
    return std::visit(visitor, variant_);
}

std::filesystem::path node::name() const {
    const auto visitor = [](auto& node) { return node.name(); };
    return std::visit(visitor, variant_);
}

node node::leaf() const {
    const auto visitor = [](auto& node) { return node.leaf(); };
    return std::visit(visitor, variant_);
}

void node::move(const std::filesystem::path& destination) {
    const auto visitor = [&destination](auto& node) { node.move(destination); };
    std::visit(visitor, variant_);
}

void node::materialize(const temp_fs& temp) const {
    const auto visitor = [&temp](auto& node) { node.materialize(temp); };
    std::visit(visitor, variant_);
}

bool operator==(const node& lhs, const node& rhs) {
    return lhs.variant_ == rhs.variant_;
}

std::ostream& operator<<(std::ostream& os, const node& node) {
    const auto visitor = [&os](auto& node) -> auto& { return os << node; };
    return std::visit(visitor, node.variant_);
}

}  // namespace glug::unit_test

