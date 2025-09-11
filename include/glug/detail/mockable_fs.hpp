#pragma once

#include <deque>
#include <filesystem>
#include <type_traits>
#include <vector>

#ifdef UNIT_TEST
#include <gmock/gmock.h>
#endif

class unmocked {};

template <typename T>
class mocked : unmocked {};

class in_mock_t {};
static constexpr inline in_mock_t in_mock{};

template <typename T>
using mockable = std::conditional_t<
        std::is_base_of_v<unmocked, mocked<T>>,
        T,
        mocked<T>>;

// TODO: Move to test/testing/
// TODO: Separate define to enable real fs testing
#ifdef UNIT_TEST
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

namespace glug::filesystem {
class access;
};

// TODO: Separate singleton class
class access_mock {
    public:
    access_mock() {
        EXPECT_EQ(std::exchange(p, this), nullptr)
                << "Multiple singletons for " << typeid(this).name();
    }
    ~access_mock() { EXPECT_EQ(std::exchange(p, nullptr), this); }
    access_mock(const access_mock&) = delete;
    access_mock(access_mock&&) = delete;
    access_mock& operator=(const access_mock&) = delete;
    access_mock& operator=(access_mock&&) = delete;

    static access_mock& instance() {
        EXPECT_NE(p, nullptr) << "No singleton for " << typeid(p).name();
        return *p;
    }

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

    private:
    inline static access_mock* p = nullptr;
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
#endif

