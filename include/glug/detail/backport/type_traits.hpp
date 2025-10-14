// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

namespace glug::backport {

template <typename T>
struct type_identity {
    using type = T;
};

template <typename T>
using type_identity_t = typename type_identity<T>::type;

}  // namespace glug::backport

