#include "glug/ignore.hpp"

#include <sstream>

namespace glug::ignore {

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

filter::filter(
        const std::vector<glob::decomposition>& globs,
        const std::filesystem::path& source
) noexcept {
    // PERF: Lazy
    const auto anchor = source.has_parent_path()
            ? glob::regex_escape(source.parent_path().string()) + "/"
            : "";

    items.reserve(globs.size());
    for (const auto& glob : globs) {
        // PERF: non-anchored patterns with no fixup can skip string convert
        auto s = std::string{ glob.pattern };
        if (glob::decomposed_pattern_fixup_required(s)) {
            s = glob::decomposed_pattern_fixup(s);
        }
        if (glob.is_anchored) {
            s = anchor + std::string{ s };
        }
        items.push_back(
                ignore_item{
                    glob.is_negative,
                    glob.is_anchored,
                    glob.is_directory,
                    std::regex{ glob::to_regex(s) },
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
        const mockable<std::filesystem::directory_entry>& entry
) const noexcept {
    const auto make_decision = [this](auto it) {
        if (it == items.rend()) {
            return decision::undecided;
        } else if (it->is_negative) {
            return decision::included;
        } else {
            return decision::ignored;
        }
    };

    if constexpr (std::filesystem::path::preferred_separator == '/'
                  && std::is_same_v<std::filesystem::path::value_type, char>) {
        const auto match = [&entry](const auto& item) noexcept {
            // PERF: Avoid recalculating `filename()`
            const auto& path
                    = item.is_anchored ? entry.path() : entry.path().filename();
            return item.is_directory && !entry.is_directory()
                    ? false
                    : std::regex_match(path.c_str(), item.regex);
        };
        return make_decision(std::find_if(items.rbegin(), items.rend(), match));
    } else {
        // PERF: Move to caller to deduplicate calculations with multi-level
        // ignore
        static constexpr auto sep
                = static_cast<char>(std::filesystem::path::preferred_separator);
        auto full = entry.path().string();
        std::replace(full.begin(), full.end(), sep, '/');
        const auto file = entry.path().filename().string();

        const auto match = [&entry, &full, &file](const auto& item) noexcept {
            const auto& path = item.is_anchored ? full : file;
            return item.is_directory && !entry.is_directory()
                    ? false
                    : std::regex_match(path, item.regex);
        };
        return make_decision(std::find_if(items.rbegin(), items.rend(), match));
    }
}

}  // namespace glug::ignore

