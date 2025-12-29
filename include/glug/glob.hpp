// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#pragma once

#include <string>
#include <string_view>

namespace glug::glob {

/**
 * Decomposition of glob line into constituent parts
 * @see decompose
 */
struct decomposition {
    std::string_view pattern{};
    bool is_negative{};
    bool is_anchored{};
    bool is_directory{};
};

/**
 * Decomposes glob line into constituent parts per gitignore rules.
 *
 * All unescaped trailing whitespace is ignored.
 *
 * Values starting with unescaped '#' are treated as empty.
 *
 * Values starting with unescaped '!' are marked as negative,
 * meaning they unignore previously ignored files and directories.
 *
 * Values containing '/' before the last character are marked as anchored,
 * meaning they match relative to directory containing the .gitignore file.
 *
 * Values containing '/' as the last character are marked as directory-only,
 * meaning they should not be used for regular files.
 */
[[nodiscard]] decomposition decompose(std::string_view glob_line) noexcept;

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

}  // namespace glug::glob

