// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/backport/memory.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <tuple>

namespace glug::backport::test {

struct int_factory {
    // NOLINTNEXTLINE
    MOCK_METHOD(bool, create, (int**));
    // NOLINTNEXTLINE
    MOCK_METHOD(void, destroy, (int*));
};

// Deleter is part of type of `unique_ptr` and therefore `out_ptr_t`, so cannot
// use lambdas as the tests' coverage would split between the instantiations.
struct deleter {
    int_factory* instance{};
    void operator()(int* p) const { instance->destroy(p); }
};

// NOLINTNEXTLINE
TEST(out_ptr_test, created) {
    auto mock = testing::StrictMock<int_factory>{};
    auto smart = std::unique_ptr<int, deleter>{};

    static constexpr int value = 123;
    EXPECT_CALL(mock, create).WillOnce([](int** p) {
        *p = std::make_unique<int>(value).release();
        return true;
    });
    EXPECT_TRUE(mock.create(glug::backport::out_ptr(smart, deleter{ &mock })));
    EXPECT_TRUE(smart);
    EXPECT_EQ(*smart, value);
    EXPECT_CALL(mock, destroy).WillOnce([](int* p) {
        std::unique_ptr<int>{ p };
    });
}

// NOLINTNEXTLINE
TEST(out_ptr_test, skipped) {
    auto mock = testing::StrictMock<int_factory>{};
    auto smart = std::unique_ptr<int, deleter>{};

    EXPECT_CALL(mock, create).WillOnce([](int** p) {
        std::ignore = p;
        return false;
    });
    EXPECT_FALSE(mock.create(glug::backport::out_ptr(smart, deleter{ &mock })));
    EXPECT_FALSE(smart);
    EXPECT_CALL(mock, destroy).Times(0);
}

}  // namespace glug::backport::test

