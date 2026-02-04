// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/program_options.hpp"

#include "glug/backport/ranges.hpp"

#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <CLI/CLI.hpp>

namespace glug::program {

namespace {

// Heavy use of references makes `CLI::App` non-moveable
struct impl {
    CLI::App app{};
    program_options options{};
    std::optional<std::string> positional{};
};

// NOLINTNEXTLINE(readability-function-size): Exceeds size due to help strings
auto make_impl() {
    auto result = std::make_unique<impl>();
    auto& [app, options, positional] = *result;
    app.description(
            "Searches paths for lines matching given patterns. Paths that "
            "are "
            "directories are recursively enumerated, using any encountered "
            ".gitignore files as filter."
    );

    app.add_option("PATTERN", positional, "Search for lines matching PATTERN.")
            ->option_text(" ");
    // TODO: Validate CLI::ExistingPath
    app.add_option(
               "PATH",
               options.paths,
               "Search files in given PATH, defaults to current directory."
    )
            ->option_text(" ");
    app.add_option(
               "-e,--regexp",
               options.patterns,
               "Search for lines matching PATTERN. Can be used to specify "
               "multiple patterns, or ones starting with a dash."
    )
            ->option_text("PATTERN")
            ->type_size(1)
            ->allow_extra_args(false);
    app.add_flag(
               "-E,--no-regexp,--list",
               options.list,
               "Print all files that would be searched."
    )
            ->excludes("--regexp");
    app.add_option(
               "-f,--filter",
               options.filters,
               "Only search in files that match given filter."
    )
            ->option_text("FILTER")
            ->type_size(1)
            ->allow_extra_args(false);

    // Don't want special throwing behavior
    app.set_help_flag("", "");
    app.add_flag("--help", options.help)->group("HELP");
    app.add_flag("--version", options.version)->group("HELP");
    app.add_flag("--license", options.license)->group("HELP");

    return result;
}  // GCOVR_EXCL_LINE

}  // namespace

program_options program_options::parse(std::span<const std::string_view> args) {
    auto impl = make_impl();
    auto& [app, options, positional] = *impl;

    // `app.parse(vector)` expects reversed args for use as a stack
    auto vargs = args | std::ranges::views::reverse
            | backport::ranges::to<std::vector<std::string>>();

    try {
        app.parse(std::move(vargs));
    } catch (const CLI::ExcludesError& e) {  // GCOVR_EXCL_LINE
        throw exclude_error{ e.what() };
    }

    if (options.help || options.version || options.license) {
        return options;
    }

    const auto emplace_front = [](auto& container, auto&& value) {
        container.emplace(
                container.begin(), std::forward<decltype(value)>(value)
        );
    };
    if (positional) {
        if (!options.list && options.patterns.empty()) {
            emplace_front(options.patterns, std::move(*positional));
        } else {
            emplace_front(options.paths, std::move(*positional));
        }
    }

    // `CLI::OptionGroup` does not work with positionals, needs manual check
    if (!options.list && options.patterns.empty()) {
        throw require_error{
            "Exactly 1 option from [PATTERN,--regexp,--no-regexp] is required"
        };
    }

    return options;
}

std::string program_options::get_help() { return make_impl()->app.help(); }

}  // namespace glug::program

