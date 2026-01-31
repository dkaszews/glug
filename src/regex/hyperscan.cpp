// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#if defined(GLUG_REGEX_HYPERSCAN)

#include "glug/generated/licenses/hyperscan.hpp"
#include "glug/regex.hpp"

#include <hs/hs_common.h>
#include <hs/hs_compile.h>
#include <hs/hs_runtime.h>

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace glug::regex {

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
    auto* db = std::type_identity_t<hs_database*>{};
    auto* error = std::type_identity_t<hs_compile_error*>{};
    [[maybe_unused]] auto result = hs_compile(
            ("^(" + std::string{ pattern } + ")$").c_str(),
            HS_FLAG_UTF8 | HS_FLAG_SINGLEMATCH,
            HS_MODE_BLOCK,
            nullptr,
            &db,
            &error
    );
    assert(result == HS_SUCCESS);

    auto* scratch = std::type_identity_t<hs_scratch*>{};
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
    [[maybe_unused]] auto result = hs_scan(
            pimpl->db.get(),
            s.data(),
            static_cast<unsigned int>(s.size()),
            0,
            pimpl->scratch.get(),
            handler,
            &found
    );
    assert(result == HS_SUCCESS || result == HS_SCAN_TERMINATED);
    return found;
}

std::string_view engine::license() {
    static const auto s = std::string{ "--- hyperscan license ---\n\n" }
            + glug::generated::licenses::hyperscan::data;
    return s;
}

}  // namespace glug::regex

#endif

