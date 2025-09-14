#pragma once

#include "glug/detail/mockable.hpp"

#include <filesystem>

namespace glug::unit_test {

template <>
class mocked<std::filesystem::directory_entry> {
    private:
    auto tie() const { return std::tie(p, dir); }

    public:
    mocked(in_mock_t, const std::filesystem::path& path, bool is_directory) :
        p{ path },
        dir{ is_directory } {}

    const auto& path() const { return p; }
    bool is_directory() const { return dir; }

    friend bool operator==(const mocked& lhs, const mocked& rhs) {
        return lhs.tie() == rhs.tie();
    }

    friend bool operator<(const mocked& lhs, const mocked& rhs) {
        return lhs.tie() < rhs.tie();
    }

    friend std::ostream& operator<<(std::ostream& os, const mocked& obj) {
        return os << obj.p << (obj.dir ? "_d" : "_f");
    }

    private:
    std::filesystem::path p{};
    bool dir{};
};

inline mocked<std::filesystem::directory_entry>
operator""_f(const char* path, size_t) {
    return { in_mock, std::filesystem::path{ path }, false };
}

inline mocked<std::filesystem::directory_entry>
operator""_d(const char* path, size_t) {
    return { in_mock, std::filesystem::path{ path }, true };
}

}  // namespace glug::unit_test

