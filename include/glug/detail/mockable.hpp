#pragma once

#include <type_traits>

namespace glug::unit_test {

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

}  // namespace glug::unit_test

namespace glug {

template <typename T>
using mockable = glug::unit_test::mockable<T>;

}  // namespace glug

