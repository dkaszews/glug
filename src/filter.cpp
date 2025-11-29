// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filter.hpp"

#include "glug/glob.hpp"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace glug::glob {

std::ostream& operator<<(std::ostream& os, decision value) noexcept {
    switch (value) {  // GCOVR_EXCL_LINE: enum out of range is UB
        case decision::undecided:
            return os << "undecided";
        case decision::ignored:
            return os << "ignored";
        case decision::included:
            return os << "included";
    }
    return os;  // GCOVR_EXCL_LINE
}

namespace {

decltype(auto) fix_path_separator(const std::filesystem::path& path) noexcept {
    static_assert(std::filesystem::path::preferred_separator == '/');
    if constexpr (std::filesystem::path::preferred_separator == '/'
                  && std::is_same_v<std::filesystem::path::value_type, char>) {
        return path;
    } else {
        return std::filesystem::path{
            path.generic_string<std::filesystem::path::value_type>()
        };
    }
}

}  // namespace

filter::filter(
        const std::vector<glob::decomposition>& globs,
        const std::filesystem::path& source
) noexcept {
    // PERF: Lazy
    // GCOVR_EXCL_START - Unknown exception paths
    auto anchor = source.has_parent_path()
            ? glob::glob_escape(
                      fix_path_separator(source.parent_path()).string()
              ) + "/"
            : "";
    // GCOVR_EXCL_STOP

    items.reserve(globs.size());
    for (const auto& glob : globs) {
        // PERF: non-anchored patterns with no fixup can skip string convert
        auto s = std::string{ glob.pattern };
        if (glob::decomposed_pattern_fixup_required(s)) {
            s = glob::decomposed_pattern_fixup(s);
        }
        if (glob.is_anchored) {
            // False positive, this is prepend instead of append
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            s = anchor + s;
        }
        items.push_back(
                ignore_item{
                    glob.is_negative,
                    glob.is_anchored,
                    glob.is_directory,
                    regex::engine{ glob::to_regex(s) },
                }
        );
    }
}

filter::filter(
        const std::vector<std::string_view>& globs,
        const std::filesystem::path& source
) noexcept :
    filter{
        [&globs]() {
            // PERF: Temporary vector created
            auto result = std::vector<glob::decomposition>{};
            result.reserve(globs.size());
            std::transform(
                    globs.begin(),
                    globs.end(),
                    std::back_inserter(result),
                    &glob::decompose
            );
            return result;
        }(),
        source,
    } {}

decision filter::is_ignored(
        const std::filesystem::directory_entry& entry
) const noexcept {
    const auto make_decision = [&items = items](const auto& it) {
        if (it == items.rend()) {
            return decision::undecided;
        }
        return it->is_negative ? decision::included : decision::ignored;
    };

    // PERF: Move to caller to deduplicate calculations with multi-level ignore
    const auto& full = fix_path_separator(entry.path());
    const auto& file = entry.path().filename();
    const auto match = [&entry, &full, &file](const auto& item) noexcept {
        const auto& path = item.is_anchored ? full : file;
        return item.is_directory && !entry.is_directory()
                ? false
                : item.regex(path.string());
    };
    return make_decision(std::find_if(items.rbegin(), items.rend(), match));
}

}  // namespace glug::glob

