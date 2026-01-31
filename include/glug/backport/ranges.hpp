// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#pragma once

#include <iostream>
#include <ranges>

namespace glug::backport::ranges {

namespace detail {

template <typename C>
C make_container(std::ranges::input_range auto&& range) {
    auto container = C{};

    if constexpr (requires { container.reserve(std::ranges::size(range)); }) {
        container.reserve(std::ranges::size(range));
    }

    for (auto&& element : range) {
        container.insert(container.end(), element);
    }

    return container;
}  // GCOVR_EXCL_LINE: noexcept RVO

template <typename C>
class to_adaptor {};

template <std::ranges::input_range R, typename C>
auto operator|(R&& range, [[maybe_unused]] to_adaptor<C>&& adaptor) {
    std::ignore = adaptor;
    return make_container<C>(std::forward<decltype(range)>(range));
}

template <template <typename...> typename C>
class to_adaptor_tt {};

template <std::ranges::input_range R, template <typename...> typename C>
auto operator|(R&& range, [[maybe_unused]] to_adaptor_tt<C>&& adaptor) {
    using t = std::ranges::iterator_t<R>::value_type;
    return make_container<C<t>>(std::forward<decltype(range)>(range));
}

}  // namespace detail

template <typename C>
constexpr auto to() {
    return detail::to_adaptor<C>{};
};

template <template <typename...> typename C>
constexpr auto to() {
    return detail::to_adaptor_tt<C>{};
};

}  // namespace glug::backport::ranges

