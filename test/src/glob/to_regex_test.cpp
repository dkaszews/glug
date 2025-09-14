#include "glug/glob.hpp"

#include "glug/test/parametrized.hpp"

#include <gtest/gtest.h>

namespace glug::glob::unit_test {

enum class affix {
    none = 0x0,
    prefix = 0x1,
    suffix = 0x2,
    both = prefix | suffix,
};

static constexpr auto operator&(affix lhs, affix rhs) {
    constexpr auto val = [](auto x) { return static_cast<size_t>(x); };
    return static_cast<affix>(val(lhs) & val(rhs));
}

struct to_regex_param {
    std::string glob = {};
    std::string expected = {};
    affix test_affix = affix::both;

    friend void PrintTo(const to_regex_param& param, std::ostream* os) {
        // Use `make_tuple` instead of `tie` to avoid gtest printing address
        return testing::internal::PrintTo(
                std::make_tuple(param.glob, param.expected), os
        );
    }
};

class to_regex_test : public testing::TestWithParam<to_regex_param> {};

TEST_P(to_regex_test, test) {
    const auto [glob, expected, test_affix] = GetParam();
    ASSERT_EQ(to_regex(glob), expected);

    if ((test_affix & affix::prefix) != affix::none) {
        EXPECT_EQ(to_regex("x" + glob), "x" + expected);
        EXPECT_EQ(to_regex("xx" + glob), "xx" + expected);
    }

    if ((test_affix & affix::suffix) != affix::none) {
        EXPECT_EQ(to_regex(glob + "x"), expected + "x");
        EXPECT_EQ(to_regex(glob + "xx"), expected + "xx");
    }

    if (test_affix == affix::both) {
        EXPECT_EQ(to_regex("x" + glob + "x"), "x" + expected + "x");
        EXPECT_EQ(to_regex("xx" + glob + "xx"), "xx" + expected + "xx");
    }
}

INSTANTIATE_TEST_SUITE_P(
        literal,
        to_regex_test,
        values_in<to_regex_param>({
            { "", "" },
            { "a", "a" },
            { "ab", "ab" },
            { "abc", "abc" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        escaped_literal,
        to_regex_test,
        values_in<to_regex_param>({
            { " ", "\\ " },
            { "#", "\\#" },
            { "$", "\\$" },
            { "&", "\\&" },
            { "(", "\\(" },
            { ")", "\\)" },
            { "+", "\\+" },
            { "-", "\\-" },
            { ".", "\\." },
            { "[", "\\[" },
            { "\\", "\\\\" },
            { "]", "\\]" },
            { "^", "\\^" },
            { "{", "\\{" },
            { "|", "\\|" },
            { "}", "\\}" },
            { "~", "\\~" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        question_mark,
        to_regex_test,
        values_in<to_regex_param>({
            { "?", "[^/]" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        star,
        to_regex_test,
        values_in<to_regex_param>({
            { "*", "[^/]+", affix::none },
            { "/*", "/[^/]+", affix::prefix },
            { "*/", "[^/]+/", affix::suffix },
            { "/*/", "/[^/]+/" },
            { "a*", "a[^/]*" },
            { "*a", "[^/]*a" },
            { "a*b", "a[^/]*b" },
            { "a/*", "a/[^/]+", affix::prefix },
            { "*/a", "[^/]+/a", affix::suffix },
            { "a/*/b", "a/[^/]+/b" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        star_star,
        to_regex_test,
        values_in<to_regex_param>({
            { "**", ".*", affix::none },
            { "a**", "a[^/]*", affix::prefix },
            { "**b", "[^/]*b", affix::suffix },
            { "a**b", "a[^/]*b" },
            { "***", "[^/]+", affix::none },
            { "/**", "/.*", affix::prefix },
            { "**/", "(.+/)?", affix::suffix },
            { "/**/", "/(.+/)?" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        set_invalid,
        to_regex_test,
        values_in<to_regex_param>({
            { "[", "\\[" },
            { "[]", "\\[\\]" },
            { "[!]", "\\[!\\]" },
            { "[/]", "\\[/\\]" },
            { "[a/]", "\\[a/\\]" },
            { "[ab/]", "\\[ab/\\]" },
            { "[abc/]", "\\[abc/\\]" },
            { "[/a]", "\\[/a\\]" },
            { "[/ab]", "\\[/ab\\]" },
            { "[/abc]", "\\[/abc\\]" },
            { "[?", "\\[\\?" },
            { "[*", "\\[\\*" },
            { "[/?]", "\\[/\\?\\]" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        set_literal,
        to_regex_test,
        values_in<to_regex_param>({
            { "[a]", "[a]" },
            { "[ab]", "[ab]" },
            { "[abc]", "[abc]" },
            { "[[]", "[\\[]" },
            { "[]]", "[\\]]" },
            { "[*]", "[\\*]" },
            { "[?]", "[\\?]" },
            { "[-]", "[\\-]" },
            { "[a-]", "[a\\-]" },
            { "[-b]", "[\\-b]" },
            { "[--]", "[\\-\\-]" },
            { "[-abc]", "[\\-abc]" },
            { "[abc-]", "[abc\\-]" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        set_range,
        to_regex_test,
        values_in<to_regex_param>({
            { "[a-c]", "[a-c]" },
            { "[a-a]", "[a-a]" },
            { "[c-a]", "[c-a]" },
            { "[a-c*]", "[a-c\\*]" },
            { "[a-?]", "[a-\\?]" },
            { "[?-c]", "[\\?-c]" },
            { "[abcx-z]", "[abcx-z]" },
            { "[a-cxyz]", "[a-cxyz]" },
            { "[a--]", "[a-\\-]" },
            { "[--%]", "[\\--%]" },
            { "[a-c-x-z]", "[a-c\\-x-z]" },
            { "[#-%]", "[\\#-%]" },
            { "[%-9]", "[%-\\.0-9]" },
            { "[.-9]", "[\\.-\\.0-9]" },
            { "[%-0]", "[%-\\.0-0]" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        set_negative,
        to_regex_test,
        values_in<to_regex_param>({
            { "[!a]", "[^/a]" },
            { "[!abc]", "[^/abc]" },
            { "[!a-c]", "[^/a-c]" },
            { "[!a-a]", "[^/a-a]" },
            { "[!c-a]", "[^/c-a]" },
            { "[!%-9]", "[^/%-9]" },
        })
);

INSTANTIATE_TEST_SUITE_P(
        mix,
        to_regex_test,
        values_in<to_regex_param>({
            { "a-cd[x--]*[!mon]", "a\\-cd[x-\\-][^/]*[^/mon]" },
            { "*-asn1.[ch]", "[^/]*\\-asn1\\.[ch]" },
            { "b[0-9]*", "b[0-9][^/]*" },
            { "*.c.[012]*.*", "[^/]*\\.c\\.[012][^/]*\\.[^/]*" },
            { "/[gmnq]conf-bin", "/[gmnq]conf\\-bin" },
            { "policy/*.conf", "policy/[^/]*\\.conf" },
            { "*.py[cod]", "[^/]*\\.py[cod]" },
            { "susp-[0-9]*-x[0-9]*", "susp\\-[0-9][^/]*\\-x[0-9][^/]*" },
        })
);

}  // namespace glug::glob::unit_test

