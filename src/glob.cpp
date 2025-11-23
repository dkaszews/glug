// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/glob.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>

using namespace std::string_literals;

namespace glug::glob {

namespace {

constexpr bool has_suffix(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size()
            && s.substr(s.size() - suffix.size()) == suffix;
};

}  // namespace

decomposition decompose(std::string_view glob) noexcept {
    if (glob.empty() || glob.front() == '#') {
        return {};
    }

    const bool is_negative = glob.front() == '!';
    glob.remove_prefix(glob.front() == '\\');
    glob.remove_prefix(is_negative);

    while (has_suffix(glob, " ") && !has_suffix(glob, "\\ ")) {
        glob.remove_suffix(1);
    }

    if (glob.empty()) {
        return {};
    }

    const bool is_anchored = glob.find('/') < glob.size() - 1;
    const bool is_directory = glob.back() == '/';
    glob.remove_suffix(is_directory);

    while (!glob.empty() && glob.front() == '/') {
        glob.remove_prefix(1);
    }

    if (glob.empty()) {
        return {};
    }

    return { glob, is_negative, is_anchored, is_directory };
}

bool decomposed_pattern_fixup_required(std::string_view pattern) noexcept {
    return !pattern.empty() && pattern.back() == ' ';
}

std::string decomposed_pattern_fixup(std::string_view pattern) noexcept {
    auto s = std::string{ pattern };
    auto trail = size_t{ 0 };
    while (has_suffix(s, "\\ ")) {
        s.resize(s.size() - 2);
        trail++;
    }
    return s + std::string(trail, ' ');
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
    const auto negative = i + 1 < glob.size() && glob[i + 1] == '!';
    const auto close = glob.find(']', i + 2 + negative);
    const auto count = close - i + 1;

    if (close == glob.npos) {
        return { regex_escape(glob.substr(i)), glob.size() - i };
    } else if (glob.find('/', i) < close) {
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

}  // namespace glug::glob

