#include "glug/glob.hpp"

#include <numeric>
#include <tuple>

using namespace std::string_literals;

namespace glug {

static const auto atom = "[^/]"s;

static auto regex_meta(char c, bool hyphen = true) noexcept {
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

static auto regex_escape(char c, bool hyphen = true) noexcept {
    return regex_meta(c, hyphen) ? "\\"s + c : std::string{ c };
}

static auto regex_escape(std::string_view s, bool hyphen = true) noexcept {
    auto result = std::string{};
    result.reserve(s.size() * 2);
    for (const auto c : s) {
        result += regex_escape(c, hyphen);
    }
    return result;
}

using skip = std::tuple<std::string, size_t>;

static void
apply_skip(std::tuple<std::string&, size_t&> acc, const skip& val) noexcept {
    auto& [acc_s, acc_i] = acc;
    const auto& [val_s, val_i] = val;
    acc_s += val_s;
    acc_i += val_i - 1;
}

static skip star_to_regex(std::string_view glob, size_t i) noexcept {
    const auto count
            = std::min(glob.find_first_not_of('*', i + 1), glob.size()) - i;
    const auto first = i == 0;
    const auto last = i + count >= glob.size();
    const auto dir_left = glob[i - 1] == '/';
    const auto dir_right = glob[i + count] == '/';
    const auto bound_left = first || dir_left;
    const auto bound_right = last || dir_right;

    if (count == 2 && bound_left && bound_right) {
        return dir_right ? skip{ "(.+/)?", count + 1 } : skip{ ".*", count };
    }

    const auto quantifier = bound_left && bound_right ? '+' : '*';
    return { atom + quantifier, count };
}

static std::string range_to_regex(std::string_view s) noexcept {
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

static skip set_to_regex(std::string_view glob, size_t i) noexcept {
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

// Only possible exceptions are allocation errors, which are neither
// plausible, nor reasonable to handle, so mark as noexcept
std::string glob_to_regex(std::string_view glob) noexcept {
    auto s = std::string{};
    for (size_t i = 0; i < glob.size(); i++) {
        auto c = glob[i];
        switch (c) {
            case '?':
                s += atom;
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

}  // namespace glug

