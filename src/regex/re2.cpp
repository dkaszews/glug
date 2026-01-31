// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#if defined(GLUG_REGEX_RE2)

#include "glug/regex.hpp"

#include <memory>
#include <string>
#include <string_view>

#include "glug/generated/licenses/re2.hpp"

#include <re2/re2.h>

namespace glug::regex {

namespace detail {

struct impl {
    explicit impl(std::string_view pattern) :
        regex{ pattern } {}

    re2::RE2 regex;
};

}  // namespace detail

engine::engine(std::string_view pattern) :
    pimpl{ std::make_shared<detail::impl>(pattern) } {}

bool engine::match(std::string_view s) const {
    return pimpl && re2::RE2::FullMatch(s, pimpl->regex);
}

std::string_view engine::license() {
    static const auto s = std::string{ "--- re2 license ---\n\n" }
            + glug::generated::licenses::re2::data;
    return s;
}

}  // namespace glug::regex

#endif

