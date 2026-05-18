// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/**
 * Classes and functions related to glob patterns.
 */
namespace glug::glob {

/**
 * Decomposition of glob with encoded metadata into constituent parts.
 *
 * @see decompose
 */
struct decomposition {
    /**
     * Glob pattern, to be applied directly or converted into regex.
     */
    std::string_view pattern{};
    /**
     * Any matched paths should be treated opposite to normal.
     *
     * @see glug::filter::decision
     */
    bool is_inverted{};
    /**
     * Glob should match entirety of path as relative to given anchor.
     *
     * If not anchored, then it matches only against path basename (leaf).
     */
    bool is_anchored{};
    /**
     * Pattern should be applied only to directories and not files.
     *
     * If false, whether it applies to both directories and files, or files
     * only, depends on `decompose_mode`.
     *
     * @see decompose_mode
     */
    bool is_directory{};
};

/**
 * Determines which mode should be used for decomposing patterns.
 *
 * @see decompose
 */
enum class decompose_mode : std::uint8_t {
    /**
     * Use exact gitignore rules.
     *
     * @see glug::filter::ignore
     */
    ignore,
    /**
     * Modified rules for quick selection of desired files.
     *
     * Similar to gitignore rules, but uses '-' instead of '!' for negation
     * and patterns without trailing '/' are not applied to directories.
     *
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
 * @param glob_line String with glob and encoded metadata.
 * @param mode Decomposition mode, modifying
 * @return Struct with raw glob and decoded flags.
 * @see decompose_mode
 */
[[nodiscard]] decomposition decompose(
        std::string_view glob_line, decompose_mode mode = decompose_mode::ignore
) noexcept;

/**
 * Splits input across occurences of given delimiter.
 *
 * Delimiter can be escaped with a backslash. Empty results are omitted.
 *
 * @param globs Delimiter-separated strings.
 * @param delimiter Character to split across.
 * @return Sequence of globs.
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
 *
 * @param glob Glob pattern.
 * @return Equivalent regex pattern.
 */
[[nodiscard]] std::string to_regex(std::string_view glob) noexcept;

/**
 * Escapes glob into literal.
 *
 * @param s String to escape.
 * @return Glob pattern which matches input literally.
 */
[[nodiscard]] std::string glob_escape(std::string_view s) noexcept;

/**
 * Database of known typetags, expanding select mode tags into multiple globs.
 */
class typetag_database {
    public:
    /**
     * Default-constructs empty database with no tags.
     */
    typetag_database() noexcept = default;

    /**
     * Constructs database mapping given tags into globs.
     *
     * @param tags Map from single tag into comma-separated globs.
     * @see split().
     */
    explicit typetag_database(
            const std::unordered_map<std::string_view, std::string_view>& tags
    );

    /**
     * @copydoc typetag_database(const std::unordered_map<std::string_view, std::string_view>&)
     */
    typetag_database(
            std::initializer_list<
                    std::pair<const std::string_view, std::string_view>> tags
    ) :
        typetag_database{
            std::unordered_map<std::string_view, std::string_view>{ tags }
        } {}

    /**
     * Expand known tags into multiple globs.
     *
     * Non-tag values and unknown tags are left as-is.
     *
     * @param globs Sequence of globs and tags.
     * @return Sequence of globs only, all tags expanded.
     *
     * @todo Unknown tags should throw.
     */
    std::vector<std::string_view>
    expand(std::span<const std::string_view> globs) const noexcept;

    /**
     * @copydoc expand()
     *
     * @todo Remove this overload.
     */
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

