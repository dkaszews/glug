// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#include "glug/glob.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace std::string_literals;

namespace glug::glob {

namespace {

constexpr bool has_suffix(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size()
            && s.substr(s.size() - suffix.size()) == suffix;
};

}  // namespace

decomposition decompose(std::string_view glob, decompose_mode mode) noexcept {
    if (glob.empty()) {
        return {};
    }
    if (mode == decompose_mode::ignore && glob.front() == '#') {
        return {};
    }

    const char inversion_char = mode == decompose_mode::ignore ? '!' : '-';
    const bool is_inverted = glob.front() == inversion_char;
    glob.remove_prefix(static_cast<size_t>(glob.front() == '\\'));
    glob.remove_prefix(static_cast<size_t>(is_inverted));

    while (has_suffix(glob, " ") && !has_suffix(glob, "\\ ")) {
        glob.remove_suffix(1);
    }

    if (glob.empty()) {
        return {};
    }

    const bool is_anchored = glob.find('/') < glob.size() - 1;
    const bool is_directory = glob.back() == '/';
    glob.remove_suffix(static_cast<size_t>(is_directory));

    while (!glob.empty() && glob.front() == '/') {
        glob.remove_prefix(1);
    }

    if (glob.empty()) {
        return {};
    }

    return {
        .pattern = glob,
        .is_inverted = is_inverted,
        .is_anchored = is_anchored,
        .is_directory = is_directory,
    };
}

std::vector<std::string_view> split(std::string_view globs, char delimiter) {
    if (globs.empty()) {
        return {};
    }

    auto result = std::vector<std::string_view>{};
    result.reserve(std::count(globs.begin(), globs.end(), delimiter));

    auto current_offset = size_t{ 0 };
    auto current_size = size_t{ 0 };
    auto escaped = false;
    for (const auto c : globs) {
        if (c == '\\') {
            current_size++;
            escaped = !escaped;
            continue;
        }

        if (escaped || c != delimiter) {
            current_size++;
            escaped = false;
            continue;
        }

        if (current_size != 0) {
            result.push_back(globs.substr(current_offset, current_size));
        }

        current_offset += current_size + 1;
        current_size = 0;
    }
    if (current_size != 0) {
        result.push_back(globs.substr(current_offset, current_size));
    }

    return result;
}

namespace {

auto regex_meta(char c, bool hyphen = true) noexcept {
    switch (c) {
        case ' ':
        case '#':
        case '$':
        case '&':
        case '(':
        case ')':
        case '*':
        case '+':
        case '.':
        case '?':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '{':
        case '|':
        case '}':
        case '~':
            return true;
        case '-':
            return hyphen;
        default:
            return false;
    }
}

auto regex_escape(char c, bool hyphen = true) noexcept {
    // NOLINTNEXTLINE(misc-include-cleaner): FP
    return regex_meta(c, hyphen) ? "\\"s + c : std::string{ c };
}

std::string regex_escape(std::string_view s, bool hyphen = true) noexcept {
    auto result = std::string{};
    result.reserve(s.size() * 2);
    for (const auto c : s) {
        result += regex_escape(c, hyphen);
    }
    return result;
}

using skip = std::tuple<std::string, size_t>;

void apply_skip(
        std::tuple<std::string&, size_t&> acc, const skip& val
) noexcept {
    auto& [acc_s, acc_i] = acc;
    const auto& [val_s, val_i] = val;
    acc_s += val_s;
    acc_i += val_i - 1;
}

skip star_to_regex(std::string_view glob, size_t i) noexcept {
    const auto count
            = std::min(glob.find_first_not_of('*', i + 1), glob.size()) - i;
    const auto first = i == 0;
    const auto last = i + count >= glob.size();
    const auto dir_left = !first && glob[i - 1] == '/';
    const auto dir_right = !last && glob[i + count] == '/';
    const auto bound_left = first || dir_left;
    const auto bound_right = last || dir_right;

    if (count == 2 && bound_left && bound_right) {
        return dir_right ? skip{ "(.+/)?", count + 1 } : skip{ ".*", count };
    }

    const auto quantifier = bound_left && bound_right ? '+' : '*';
    return { "[^/]"s + quantifier, count };
}

std::string range_to_regex(std::string_view s) noexcept {
    auto result = std::string{};
    result.reserve(s.size() * 2);

    for (size_t i = 0; i < s.size(); i++) {
        if (i == s.size() - 1 || s[i + 1] != '-') {
            result += regex_escape(s[i]);
            continue;
        }

        const auto from = s[i];
        const auto to = s[i += 2];
        if (from > to || from > '/' || to < '/') {
            result += regex_escape(from) + '-' + regex_escape(to);
            continue;
        }

        result += regex_escape(from) + '-' + regex_escape('/' - 1)
                + regex_escape('/' + 1) + '-' + regex_escape(to);
    }
    return result;
}

skip set_to_regex(std::string_view glob, size_t i) noexcept {
    constexpr auto minimum_length = size_t{ 2 };  // One char + close bracket
    const auto negative = i + 1 < glob.size() && glob[i + 1] == '!';
    const auto close = glob.find(']', i + minimum_length + (negative ? 1 : 0));
    const auto count = close - i + 1;

    if (close == glob.npos) {
        return { regex_escape(glob.substr(i)), glob.size() - i };
    }
    if (glob.find('/', i) < close) {
        return { regex_escape(glob.substr(i, count)), count };
    }

    const auto inner = glob.substr(i + 1, count - 2);
    if (negative) {
        return { "[^/" + regex_escape(inner.substr(1), false) + "]", count };
    }

    return (glob.find('-', i + 2) > close - 2)
            ? skip{ "[" + regex_escape(inner) + "]", count }
            : skip{ "[" + range_to_regex(inner) + "]", count };
}

}  // namespace

// Only possible exceptions are allocation errors, which are neither
// plausible, nor reasonable to handle, so mark as noexcept
std::string to_regex(std::string_view glob) noexcept {
    auto s = std::string{};
    for (size_t i = 0; i < glob.size(); i++) {
        auto c = glob[i];
        switch (c) {
            case '\\':
                s += '\\';
                s += i < glob.size() - 1 ? glob[++i] : '\\';
                break;
            case '?':
                s += "[^/]"s;
                break;
            case '*':
                apply_skip(std::tie(s, i), star_to_regex(glob, i));
                break;
            case '[':
                apply_skip(std::tie(s, i), set_to_regex(glob, i));
                break;
            default:
                s += regex_escape(c);
                break;
        }
    }
    return s;
}

std::string glob_escape(std::string_view s) noexcept {
    auto result = std::string{};
    result.reserve(s.size() * 2);
    for (const auto c : s) {
        switch (c) {
            case '?':
            case '*':
            case '[':
                result += '\\';
                break;
            default:
                break;
        }
        result += c;
    }
    return result;
}

typetag_database::typetag_database(
        const std::unordered_map<std::string_view, std::string_view>& tags
) {
    const auto stringify = [](const auto& views) {
        auto result = std::vector<std::string>{ views.begin(), views.end() };
        return result;
    };

    const auto negate = [](const auto& positive) noexcept {
        auto negative = positive;
        for (auto& value : negative) {
            value = std::string{ "-" }.append(value);
        }
        return negative;
    };

    map.reserve(tags.size());
    for (const auto& [key, value] : tags) {
        const auto positive = stringify(split(value));
        const auto negative = negate(positive);
        map[key] = { .positive = positive, .negative = negative };
    }
}  // GCOVR_EXCL_LINE: Unknown exceptional path

std::vector<std::string_view> typetag_database::expand(
        const std::vector<std::string_view>& globs
) const noexcept {
    auto result = std::vector<std::string_view>{};
    for (auto glob : globs) {
        if (!glob.starts_with("#") && !glob.starts_with("-#")) {
            result.push_back(glob);
            continue;
        }

        const auto inverted = glob.front() == '-';
        const auto tag = glob.substr(static_cast<size_t>(inverted) + 1);
        const auto it = map.find(tag);
        if (it == map.end()) {
            result.push_back(glob);
            continue;
        }

        const auto& values
                = inverted ? it->second.negative : it->second.positive;
        result.insert(result.end(), values.begin(), values.end());
    }
    return result;
}

}  // namespace glug::glob

