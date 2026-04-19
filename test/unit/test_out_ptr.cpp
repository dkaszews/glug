// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/backport/memory.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <tuple>

namespace glug::backport::test {

struct int_factory {
    MOCK_METHOD(bool, create, (int**));
    MOCK_METHOD(void, destroy, (int*));
};

// Deleter is part of type of `unique_ptr` and therefore `out_ptr_t`, so cannot
// use lambdas as the tests' coverage would split between the instantiations.
struct deleter {
    int_factory* instance{};
    // NOLINTNEXTLINE(bugprone-exception-escape): Only throws for default action
    void operator()(int* p) const noexcept { instance->destroy(p); }
};

TEST(out_ptr_test, created) {
    auto mock = testing::StrictMock<int_factory>{};
    auto smart = std::unique_ptr<int, deleter>{};

    int value = 3;
    EXPECT_CALL(mock, create).WillOnce([&value](int** p) {
        *p = &value;
        return true;
    });
    EXPECT_TRUE(mock.create(glug::backport::out_ptr(smart, deleter{ &mock })));
    EXPECT_TRUE(smart);
    EXPECT_EQ(*smart, value);
    EXPECT_CALL(mock, destroy);
}

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

