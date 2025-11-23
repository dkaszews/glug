// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/regex.hpp"

#include <memory>
#include <string_view>

#if defined(ENGINE_PCRE2)
#include <pcre2.h>
#elif defined(ENGINE_RE2)
#include <re2/re2.h>
#elif defined(ENGINE_HYPERSCAN)
#include <hs/hs.h>

#include <cassert>
#include <string>
#else
#include <regex>
#endif

namespace glug::regex {

#if defined(ENGINE_PCRE2)

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
            reinterpret_cast<const unsigned char*>(s.data()),
            s.size(),
            0,
            PCRE2_ANCHORED | PCRE2_ENDANCHORED,
            pimpl->scratch.get(),
            nullptr
    );
    return matches >= 0;
}

#elif defined(ENGINE_RE2)

namespace detail {

struct impl {
    impl(std::string_view pattern) :
        regex{ pattern } {}

    re2::RE2 regex;
};

}  // namespace detail

engine::engine(std::string_view pattern) :
    pimpl{ std::make_shared<detail::impl>(pattern) } {}

bool engine::match(std::string_view s) const {
    return pimpl && re2::RE2::FullMatch(s, pimpl->regex);
}

#elif defined(ENGINE_HYPERSCAN)

namespace detail {

struct impl {
    struct database_deleter {
        void operator()(hs_database* p) const { hs_free_database(p); }
    };
    struct scratch_deleter {
        void operator()(hs_scratch* p) const { hs_free_scratch(p); }
    };

    impl(hs_database* db, hs_scratch* scratch) :
        db{ db, {} },
        scratch{ scratch, {} } {}

    std::unique_ptr<hs_database, database_deleter> db{};
    std::unique_ptr<hs_scratch, scratch_deleter> scratch{};
};

}  // namespace detail

engine::engine(std::string_view pattern) {
    // TODO: #15 - backport std::out_ptr
    auto db = backport::type_identity_t<hs_database*>{};
    auto error = backport::type_identity_t<hs_compile_error*>{};
    [[maybe_unused]] auto result = hs_compile(
            ("^(" + std::string{ pattern } + ")$").c_str(),
            HS_FLAG_UTF8 | HS_FLAG_SINGLEMATCH,
            HS_MODE_BLOCK,
            nullptr,
            &db,
            &error
    );
    assert(result == HS_SUCCESS);

    auto scratch = backport::type_identity_t<hs_scratch*>{};
    result = hs_alloc_scratch(db, &scratch);
    assert(result == HS_SUCCESS);
    pimpl = std::make_shared<detail::impl>(db, scratch);
}

bool engine::match(std::string_view s) const {
    if (!pimpl) {
        return false;
    }

    auto found = false;
    const auto handler = [](auto, auto, auto, auto, void* found) -> int {
        return *static_cast<bool*>(found) = true;
    };
    auto db = pimpl->db.get();
    auto scratch = pimpl->scratch.get();
    hs_scan(db, s.data(), s.size(), 0, scratch, handler, &found);
    return found;
}

#else

namespace detail {

struct impl {
    // NOLINTNEXTLINE(google-explicit-constructor): Desired implicit
    impl(std::string_view s) :
        re{ s.data(), s.size() } {}

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes): FP
    std::regex re{};
};

}  // namespace detail

engine::engine(std::string_view pattern) :
    pimpl{ std::make_shared<detail::impl>(pattern) } {}

bool engine::match(std::string_view s) const {
    return std::regex_match(s.begin(), s.end(), pimpl->re);
}

#endif

}  // namespace glug::regex

