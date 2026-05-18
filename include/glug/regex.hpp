// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include <memory>
#include <string_view>

/**
 * Regular expression utilities.
 */
namespace glug::regex {

/**
 * Wrapper around regex engine, which can be provided by either STL
 * or third-party libraries, depending on project config.
 */
class engine {
    public:
    /*
     * Default-construct a regex matcher, which does not match anything.
     *
     * @todo: UT if this is true, might actually be UB.
     */
    engine() = default;

    /**
     * Construct regex matcher from given regex.
     *
     * @param pattern regular expression
     * @throws std::exception if pattern is not a valid regex.
     *
     * @todo: Add custom exception class.
     * @todo: UT exceptions.
     */
    explicit engine(std::string_view pattern);

    /**
     * Check given string against the matcher.
     *
     * @param s string for matching against the regex.
     * @return True if entire string matches, false otherwise.
     */
    [[nodiscard]] bool match(std::string_view s) const;

    /**
     * Alias for `engine::match(std::string_view)`.
     *
     * @param s string for matching against the regex.
     * @return True if entire string matches, false otherwise.
     *
     * @see engine::match(std::string_view)
     */
    [[nodiscard]] bool operator()(std::string_view s) const { return match(s); }

    // TODO: #90 - Generate version header
    /// @cond
    static std::string_view license();
    /// @endcond

    private:
    struct impl;
    std::shared_ptr<impl> pimpl{};
};

};  // namespace glug::regex

