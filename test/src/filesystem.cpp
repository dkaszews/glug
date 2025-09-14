#include "glug/test/filesystem.hpp"

#include "glug/detail/mockable/access.hpp"

#include <filesystem>
#include <fstream>
#include <random>

#include <gtest/gtest.h>

namespace glug::unit_test {

std::filesystem::path create_temp_dir() {
    const auto temp_dir = std::filesystem::temp_directory_path();
    auto rng = std::mt19937{ std::random_device{}() };
    std::uniform_int_distribution<uint64_t> distribution{};
    while (true) {
        const auto path = temp_dir / std::to_string(distribution(rng));
        if (std::filesystem::create_directory(path)) {
            return path;
        }
    }
    return {};
}

static bool is_dir(const std::filesystem::path& path) {
    return !path.empty() && path.string().back() == '/';
}

static std::filesystem::path strip_dir(const std::filesystem::path& path) {
    if (path == "/" || !is_dir(path)) {
        return path;
    }
    auto s = path.string();
    s.pop_back();
    return s;
}

#ifdef MOCK_FS

void tree::create(std::filesystem::path prefix) const {
    prefix = strip_dir(prefix / path);
    auto& mock = mockable<glug::filesystem::access>::mock::instance();

    if (const auto* lines = std::get_if<tree::lines>(&contents)) {
        EXPECT_CALL(mock, read_lines(prefix)).WillOnce(testing::Return(*lines));
    }

    const auto* files = std::get_if<tree::files>(&contents);
    if (!files) {
        return;
    }

    auto expected = std::deque<mockable<std::filesystem::directory_entry>>{};
    const auto transform = [&prefix](const auto& node) {
        return mockable<std::filesystem::directory_entry>{
            in_mock, prefix / strip_dir(node.path), is_dir(node.path)
        };
    };
    std::transform(
            files->begin(),
            files->end(),
            std::back_inserter(expected),
            transform
    );
    EXPECT_CALL(mock, list_directory(prefix))
            .WillOnce(testing::Return(expected));

    for (const auto& file : *files) {
        file.create(prefix);
    }
    return;
}

#else  // !MOCK_FS

void tree::create(std::filesystem::path prefix) const {
    prefix = strip_dir(prefix / path);
    if (is_dir(path)) {
        std::filesystem::create_directory(prefix);
    } else {
        std::ofstream{ prefix };
    }

    if (const auto* lines = std::get_if<tree::lines>(&contents)) {
        auto file = std::ofstream{ prefix };
        for (const auto& line : *lines) {
            file << line << "\n";
        }
    }

    if (const auto* files = std::get_if<tree::files>(&contents)) {
        for (const auto& file : *files) {
            file.create(prefix);
        }
    }
}

#endif  // MOCK_FS

static void unwind_impl(
        std::deque<mockable<std::filesystem::directory_entry>>& result,
        const tree& node,
        std::filesystem::path prefix
) {
    prefix = strip_dir(prefix / node.path);
    if (!is_dir(node.path)) {
#ifdef MOCK_FS
        result.emplace_back(in_mock, prefix, false);
#else
        result.emplace_back(prefix);
#endif
        return;
    }

    const auto* files = std::get_if<tree::files>(&node.contents);
    if (!files) {
        return;
    }

    for (const auto& file : *files) {
        unwind_impl(result, file, prefix);
    }
}

std::deque<mockable<std::filesystem::directory_entry>>
tree::unwind(std::filesystem::path prefix) const {
    auto result = std::deque<mockable<std::filesystem::directory_entry>>{};
    unwind_impl(result, *this, prefix);
    return result;
}

tree operator&(const tree& node, const tree::files& contents) {
    EXPECT_TRUE(is_dir(node.path));
    auto result = node;
    result.contents = contents;
    return result;
}

tree operator&(const tree& node, const tree::lines& contents) {
    EXPECT_FALSE(is_dir(node.path));
    auto result = node;
    result.contents = contents;
    return result;
}

}  // namespace glug::unit_test

