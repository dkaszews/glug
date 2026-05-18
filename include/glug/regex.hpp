// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include <memory>
#include <string_view>

/**
 * Classes related to regular expressions.
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
     * @param pattern Regular expression
     * @throws std::exception if pattern is not a valid regex.
     *
     * @todo: Add custom exception class.
     * @todo: UT exceptions.
     */
    explicit engine(std::string_view pattern);

    /**
     * Check given string against the matcher.
     *
     * @param s String for matching against the regex.
     * @return True if entire string matches, false otherwise.
     */
    [[nodiscard]] bool match(std::string_view s) const;

    /// @copydoc match()
    [[nodiscard]] bool operator()(std::string_view s) const { return match(s); }

    /**
     * Instances of `engine` are not comparable.
     *
     * This is because their internal compiled is generally not comparable, as
     * implementing comparison would be either expensive or require otherwise
     * wasteful tracking of the original pattern.
     */
    bool operator==(const engine&) = delete;

    // TODO: #90 - Generate version header
    /// @cond
    static std::string_view license();
    /// @endcond

    private:
    struct impl;
    std::shared_ptr<impl> pimpl{};
};

};  // namespace glug::regex

