// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#include "glug/filesystem.hpp"
#include "glug/program_options.hpp"
#include "glug/regex.hpp"

#include "glug/generated/license.hpp"
#include "glug/generated/licenses/cli11.hpp"

#include <algorithm>
#include <format>
#include <iostream>
#include <iterator>
#include <map>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace std::string_view_literals;

namespace {

template <typename... ARGS>
void print(
        std::ostream& os, std::format_string<ARGS...> format, ARGS&&... args
) {
    auto out = std::ostream_iterator<char>{ os };
    std::format_to(out, format, std::forward<ARGS>(args)...);
}

template <typename... ARGS>
void println(
        std::ostream& os, std::format_string<ARGS...> format, ARGS&&... args
) {
    print(os, format, std::forward<ARGS>(args)...);
    os << "\n";
}

template <typename... ARGS>
void print(std::format_string<ARGS...> format, ARGS&&... args) {
    print(std::cout, format, std::forward<ARGS>(args)...);
}

template <typename... ARGS>
void println(std::format_string<ARGS...> format, ARGS&&... args) {
    println(std::cout, format, std::forward<ARGS>(args)...);
}

const auto tags = std::unordered_map<std::string_view, std::string_view>{
    { "asm", "*.asm,*.[sS]" },
    { "cpp", "*.cpp,*.cc,*.cxx,*.m,*.hpp,*.hh,*.h,*.hxx" },
    { "batch", "*.bat,*.cmd" },
    { "cc", "*.c,*.h,*.xs" },
    { "cmake", "CMakeLists.txt,*.cmake" },
    { "csharp", "*.cs" },
    { "hh", "*.h" },
    { "hpp", "*.hpp,*.hh,*.h,*.hxx" },
    { "lua", "*.lua" },
    { "make", "*.mk,*.mak,[mM]akefile,GNUmakefile" },
    { "md", "*.markdown,*.mdown,*.mdwn,*.mkdn,*.mkd,*.md" },
    { "python", "*.py" },
    { "shell", "*.sh,*.bash,*.csh,*.tcsh,*.ksh,*.zsh,*.fish" },
    { "vim", "*.vim" },
};

void print_license() {
    println("--- glug license --- \n\n{}", glug::generated::license::data);
    println("--- CLI11 license --- \n\n{}",
            glug::generated::licenses::cli11::data);
    if (const auto license = glug::regex::engine::license(); !license.empty()) {
        println("{}", license);
    }
}

void print_tags() {
    const auto max_elem = std::ranges::max_element(
            tags, [](const auto& lhs, const auto& rhs) {
                return lhs.first.size() < rhs.first.size();
            }
    );
    const auto pad = max_elem->first.size();

    for (const auto& [tag, globs] : std::map(tags.begin(), tags.end())) {
        println("{:{}}  {}", tag, pad, globs);
    }
}

void print_help(const glug::program::program_options::help_flags& help) {
    if (help.show_help) {
        print("{}", glug::program::program_options::get_help());
    } else if (help.show_tags) {
        print_tags();
    } else if (help.show_version) {
        println("{}", GLUG_VERSION);
    } else if (help.show_license) {
        print_license();
    }
}

}  // namespace

// NOLINTNEXTLINE(bugprone-exception-escape): Assume `print` is correct
int main(int argc, const char** argv) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto args = std::vector<std::string_view>{ argv + 1, argv + argc };
    auto options = glug::program::program_options{};
    try {
        options = glug::program::program_options::parse(args);
    } catch (const std::exception& e) {
        println(std::cerr, "{}\nSee --help", e.what());
        return 1;
    }

    if (options.help) {
        print_help(options.help);
        return 0;
    }

    if (!options.list) {
        // TODO:
        println(std::cerr, "--regexp not implemented, --no-regexp required");
        return 1;
    }

    if (options.filters.size() > 1) {
        // TODO:
        println(std::cerr, "Repeated --filter not implemented");
        return 1;
    }

    if (options.paths.empty()) {
        options.paths.emplace_back(".");
    }

    const auto db = glug::glob::typetag_database{ tags };
    for (const auto& path : options.paths) {
        const auto select
                = !options.filters.empty() ? options.filters.front() : "";
        const auto explorer = glug::filesystem::explorer{
            path, { glug::filter::select{ db.expand(select), path } }
        };

        const auto trim_dot = path == "." ? 2 : 0;
        for (const auto& file : explorer) {
            println("{}", file.path().generic_string().substr(trim_dot));
        }
    }

    return 0;
}

