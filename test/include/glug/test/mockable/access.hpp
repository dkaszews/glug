#pragma once

#include "glug/detail/mockable.hpp"
#include "glug/test/mockable/directory_entry.hpp"
#include "glug/test/singleton.hpp"

#include <deque>
#include <filesystem>
#include <vector>

#include <gmock/gmock.h>

namespace glug::filesystem {
class access;
}  // namespace glug::filesystem

namespace glug::unit_test {

class access_mock : public singleton<access_mock> {
    public:
    MOCK_METHOD(
            std::deque<mocked<std::filesystem::directory_entry>>,
            list_directory,
            (const std::filesystem::path& path),
            (const)
    );

    MOCK_METHOD(
            std::vector<std::string>,
            read_lines,
            (const std::filesystem::path& path),
            (const)
    );
};

template <>
class mocked<glug::filesystem::access> {
    public:
    std::deque<mocked<std::filesystem::directory_entry>>
    list_directory(const std::filesystem::path& path) const {
        return access_mock::instance().list_directory(path);
    }

    std::vector<std::string>
    read_lines(const std::filesystem::path& path) const {
        return access_mock::instance().read_lines(path);
    }
};

}  // namespace glug::unit_test

