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

namespace glug::regex {

namespace detail {

struct impl {
    struct database_deleter {
        void operator()(hs_database* p) const { hs_free_database(p); }
    };
    struct scratch_deleter {
        void operator()(hs_scratch* p) const { hs_free_scratch(p); }
    };

    std::unique_ptr<hs_database, database_deleter> db{};
    std::unique_ptr<hs_scratch, scratch_deleter> scratch{};
};

}  // namespace detail

engine::engine(std::string_view pattern) :
    pimpl{ std::make_shared<detail::impl>() } {

    auto error = std::make_unique<hs_compile_error>();
    [[maybe_unused]] auto result = hs_compile(
            ("^(" + std::string{ pattern } + ")$").c_str(),
            HS_FLAG_UTF8 | HS_FLAG_SINGLEMATCH,
            HS_MODE_BLOCK,
            nullptr,
            std::out_ptr(pimpl->db),
            std::out_ptr(error)
    );
    assert(result == HS_SUCCESS);

    result = hs_alloc_scratch(pimpl->db.get(), std::out_ptr(pimpl->scratch));
    assert(result == HS_SUCCESS);
}

bool engine::match(std::string_view s) const {
    if (!pimpl) {
        return false;
    }

    auto found = false;
    const auto handler = [](auto, auto, auto, auto, void* found) -> int {
        return *static_cast<bool*>(found) = true;
    };
    auto* db = pimpl->db.get();
    auto* scratch = pimpl->scratch.get();
    const auto size = static_cast<unsigned int>(s.size());
    // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): Size variable
    hs_scan(db, s.data(), size, 0, scratch, handler, &found);
    return found;
}

std::string_view engine::license() {
    static const auto s = std::string{ "--- hyperscan license ---\n\n" }
            + glug::generated::licenses::hyperscan::data;
    return s;
}

}  // namespace glug::regex

#endif

