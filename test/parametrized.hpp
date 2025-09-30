// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

#include <gtest/gtest.h>

#include <vector>

template <typename T>
static auto values_in(const std::vector<T>& values) {
    return testing::ValuesIn(values);
}

