// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/program_options.hpp"

#include "glug/backport/ranges.hpp"

#include <optional>
#include <ranges>
#include <string>
#include <utility>

#include <CLI/CLI.hpp>

namespace glug::program {

namespace {

// Heavy use of references makes `CLI::App` non-moveable
struct impl {
    CLI::App app{};
    program_options options{};
    std::optional<std::string> positional{};
};

auto make_impl() {
    // TODO: App description
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
               options.flags.list,
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

    app.set_help_flag("--help", "")->group("HELP");
    // TODO: Maybe combine under "about"?
    app.add_flag("--help-filter", options.flags.help_filter, "")->group("HELP");
    app.add_flag("--help-tags", options.flags.help_tags, "")->group("HELP");
    app.add_flag("--version", options.flags.version)->group("HELP");
    app.add_flag("--license", options.flags.license)->group("HELP");

    return result;
}

}  // namespace

program_options program_options::parse(std::span<const std::string_view> args) {
    auto impl = make_impl();
    auto& [app, options, positional] = *impl;

    // `app.parse(vector)` expects reversed args for use as a stack
    auto vargs = args | std::ranges::views::reverse
            | backport::ranges::to<std::vector<std::string>>();

    try {
        app.parse(std::move(vargs));
    } catch (const CLI::CallForHelp& e) {
        std::ignore = e;
        return { .flags = { .help = true } };
    } catch (const CLI::RequiredError& e) {
        throw require_error{ e.what() };
    } catch (const CLI::ExcludesError& e) {
        throw exclude_error{ e.what() };
    }

    const auto emplace_front = [](auto& container, auto&& value) {
        container.emplace(container.begin(), std::move(value));
    };
    if (positional) {
        if (!options.flags.list && options.patterns.empty()) {
            emplace_front(options.patterns, std::move(*positional));
        } else {
            emplace_front(options.paths, std::move(*positional));
        }
    }

    // `CLI::OptionGroup` does not work with positionals, needs manual check
    if (!options.flags.list && options.patterns.empty()) {
        throw require_error{
            "Exactly 1 option from [PATTERN,--regexp,--no-regexpr] is required"
        };
    }

    return options;
}

std::string program_options::get_help() { return make_impl()->app.help(); }

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& container) {
    bool first = true;
    os << "[ ";
    for (const auto& value : container) {
        if (!std::exchange(first, false)) {
            os << ", ";
        }
        os << value;
    }
    return os << " ]";
}

std::ostream& operator<<(std::ostream& os, const program_flags& flags) {
    auto set_flags = std::vector<std::string>{};
    const auto add_flags = [&](bool flag, const std::string& name) {
        if (flag) {
            set_flags.emplace_back("." + name + " = true");
        }
    };
    add_flags(flags.list, "list");
    add_flags(flags.version, "version");
    add_flags(flags.license, "license");
    add_flags(flags.help, "help");
    add_flags(flags.help_filter, "help_filter");
    add_flags(flags.help_tags, "help_tags");
    return os << "flags{ " << set_flags << " }";
}

std::ostream& operator<<(std::ostream& os, const program_options& options) {
    os << "program_options";
    os << "{ .patterns = " << options.patterns;
    os << ", .paths = " << options.paths;
    os << ", .filters = " << options.filters;
    os << ", .flags = " << options.flags;
    return os << " }";
}

}  // namespace glug::program

