// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace glug::glob {

/**
 * Decomposition of glob line into constituent parts
 * @see decompose
 */
struct decomposition {
    std::string_view pattern{};
    bool is_inverted{};
    bool is_anchored{};
    bool is_directory{};
};

/**
 * Determines which mode should be used for decomposing patterns.
 * @see decompose
 */
enum class decompose_mode : std::uint8_t {
    /**
     * Use exact gitignore rules.
     */
    ignore,
    /**
     * Similar to gitignore rules, but uses '-' instead of '!' for negation
     * and patterns without trailing '/' are not applied to directories.
     * @see glug::filter::select
     */
    select,
};

/**
 * Decomposes glob line into constituent parts.
 *
 * All unescaped trailing whitespace is ignored.
 *
 * Ignore mode follows .gitignore rules. Values are negative by default, meaning
 * they cause matching files or directories to be excluded, and can be inverted
 * to positive and include previously excluded entries with '!'. Values starting
 * with unescaped '#' are comments and treated as empty.
 *
 * In select mode, values are positive by default and are inverted to negative
 * with '-' instead.
 *
 * Values containing '/' before the last character are marked as anchored,
 * meaning they match relative to directory containing the .gitignore file,
 * or search target in select mode.
 *
 * Values containing '/' as the last character are marked as directory-only,
 * meaning they should not be used for regular files. In ignore mode, values
 * not ending in '/' are applied to both files and directories. In select mode,
 * they are applied only to files.
 *
 * @see decompose_mode
 */
[[nodiscard]] decomposition decompose(
        std::string_view glob_line, decompose_mode mode = decompose_mode::ignore
) noexcept;

/**
 * Splits input across unescaped occurences of given delimiter, omitting empty
 * results.
 */
[[nodiscard]] std::vector<std::string_view>
split(std::string_view globs, char delimiter = ',');

/**
 * Converts glob pattern to equivalent regular expression per gitignore rules.
 *
 * Double asterisk "**" surrounded by path separator "/" and/or string boundary
 * can match any number of directories, including zero.
 *
 * A path separator '/' can only be matched literally, never by '?', '*',
 * "[...]" or "[!...]". Path separators are escaped from positive sets using
 * range splitting instead of negative lookahead, to allow use in even simplest
 * regex engines.
 */
[[nodiscard]] std::string to_regex(std::string_view glob) noexcept;

/**
 * Escapes a string literal into a glob expression matching only the literal.
 */
[[nodiscard]] std::string glob_escape(std::string_view s) noexcept;

/**
 * Database of known typetags, expanding select mode tags into multiple globs.
 */
class typetag_database {
    public:
    typetag_database() noexcept = default;

    explicit typetag_database(
            const std::unordered_map<std::string_view, std::string_view>& tags
    );

    typetag_database(
            std::initializer_list<
                    std::pair<const std::string_view, std::string_view>> tags
    ) :
        typetag_database{ { tags } } {}

    /**
     * Expand known tags into multiple globs.
     *
     * Non-tag values and unknown tags are left as-is.
     */
    std::vector<std::string_view>
    expand(const std::vector<std::string_view>& globs) const noexcept;

    std::vector<std::string_view>
    expand(std::string_view globs) const noexcept {
        return expand(split(globs));
    }

    private:
    struct mapping {
        std::vector<std::string> positive{};
        std::vector<std::string> negative{};
    };
    std::unordered_map<std::string_view, mapping> map{};
};

}  // namespace glug::glob

