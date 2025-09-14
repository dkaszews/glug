#include "glug/test/filesystem.hpp"

namespace glug::unit_test {

void test_fs::create(access_mock& mock, std::filesystem::path prefix) const {
    prefix /= entry.path();

    if (const auto* lines = std::get_if<test_fs::lines>(&contents)) {
        EXPECT_CALL(mock, read_lines(prefix)).WillOnce(testing::Return(*lines));
    }

    const auto* files = std::get_if<test_fs::files>(&contents);
    if (!files) {
        return;
    }

    auto expected = std::deque<directory_entry>{};
    const auto transform = [&](const auto& entry) {
        return directory_entry{
            in_mock,
            prefix / entry.entry.path(),
            entry.entry.is_directory(),
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
        file.create(mock, prefix);
    }
}

static void unwind_impl(
        std::deque<test_fs::directory_entry>& result,
        const test_fs& tree,
        std::filesystem::path prefix
) {
    prefix /= tree.entry.path();
    if (!tree.entry.is_directory()) {
        result.emplace_back(in_mock, prefix, false);
        return;
    }

    const auto* files = std::get_if<test_fs::files>(&tree.contents);
    if (!files) {
        return;
    }

    for (const auto& file : *files) {
        unwind_impl(result, file, prefix);
    }
}

std::deque<test_fs::directory_entry>
test_fs::unwind(std::filesystem::path prefix) const {
    auto result = std::deque<directory_entry>{};
    unwind_impl(result, *this, prefix);
    return result;
}

test_fs operator/(const test_fs& tree, const test_fs::files& contents) {
    auto result = tree;
    result.contents = contents;
    return result;
}

test_fs operator/(const test_fs& tree, const test_fs::lines& contents) {
    auto result = tree;
    result.contents = contents;
    return result;
}

}  // namespace glug::unit_test

