// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#if defined(GLUG_REGEX_STL)

#include "glug/regex.hpp"

#include <memory>
#include <regex>
#include <string_view>

namespace glug::regex {

namespace detail {

struct impl {
    // NOLINTNEXTLINE(google-explicit-constructor): Desired implicit
    impl(std::string_view s) :
        re{ s.data(), s.size() } {}

    std::regex re{};
};

}  // namespace detail

engine::engine(std::string_view pattern) :
    pimpl{ std::make_shared<detail::impl>(pattern) } {}

bool engine::match(std::string_view s) const {
    return std::regex_match(s.begin(), s.end(), pimpl->re);
}

std::string_view engine::license() { return ""; }

}  // namespace glug::regex

#endif

