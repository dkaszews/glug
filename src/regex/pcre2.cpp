// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#if defined(GLUG_REGEX_PCRE2)

#include "glug/regex.hpp"

#include "glug/generated/licenses/pcre2.hpp"

#include <pcre2.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace glug::regex {

namespace detail {

struct impl {
    struct code_deleter {
        void operator()(pcre2_code_8* p) const { pcre2_code_free_8(p); };
    };
    struct scratch_deleter {
        void operator()(pcre2_match_data_8* p) const {
            pcre2_match_data_free_8(p);
        };
    };

    std::unique_ptr<pcre2_code_8, code_deleter> code{};
    std::unique_ptr<pcre2_match_data_8, scratch_deleter> scratch{};
};

}  // namespace detail

engine::engine(std::string_view pattern) :
    pimpl{ std::make_shared<detail::impl>() } {

    auto error_code = int{};
    auto error_offset = size_t{};

    pimpl->code = {
        pcre2_compile_8(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                reinterpret_cast<const unsigned char*>(pattern.data()),
                pattern.size(),
                PCRE2_UTF,
                &error_code,
                &error_offset,
                nullptr
        ),
        {},
    };
    if (!pimpl->code) {
        pimpl = nullptr;
        return;
    }

    pimpl->scratch = {
        pcre2_match_data_create_from_pattern(pimpl->code.get(), nullptr),
        {},
    };
}

bool engine::match(std::string_view s) const {
    if (!pimpl) {
        return false;
    }

    const auto matches = pcre2_match_8(
            pimpl->code.get(),
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const unsigned char*>(s.data()),
            s.size(),
            0,
            PCRE2_ANCHORED | PCRE2_ENDANCHORED,
            pimpl->scratch.get(),
            nullptr
    );
    return matches >= 0;
}

std::string_view engine::license() {
    static const auto s = std::string{ "--- pcre2 license ---\n\n" }
            + glug::generated::licenses::pcre2::data;
    return s;
}

}  // namespace glug::regex

#endif

