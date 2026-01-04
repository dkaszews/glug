// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#include "glug/filesystem.hpp"

#include "glug/generated/license.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>

#if defined(GLUG_REGEX_PCRE2)
#include "glug/generated/licenses/pcre2.hpp"
void print_regex_license() {
    std::cout << "--- pcre2 license ---\n\n";
    std::cout << glug::generated::licenses::pcre2::data;
}
#elif defined(GLUG_REGEX_RE2)
#include "glug/generated/licenses/re2.hpp"
void print_regex_license() {
    std::cout << "--- re2 license ---\n\n";
    std::cout << glug::generated::licenses::re2::data;
}
#elif defined(GLUG_REGEX_HYPERSCAN)
#include "glug/generated/licenses/hyperscan.hpp"
void print_regex_license() {
    std::cout << "--- hyperscan license ---\n\n";
    std::cout << glug::generated::licenses::hyperscan::data;
}
#else
void print_regex_license() {}
#endif

const auto db = glug::glob::typetag_database{
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

int print_help() {
    std::cout << "TODO: help!\n";
    return 0;
}

int print_version() {
    std::cout << GLUG_VERSION << "\n";
    return 0;
}

int print_license() {
    std::cout << "--- glug license --- \n\n";
    std::cout << glug::generated::license::data << "\n";
    print_regex_license();
    return 0;
}

int print_tags() {
    std::cout << "TODO: tags\n";
    return 0;
}

int main(int argc, const char** argv) {
    using namespace std::string_view_literals;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto args = std::vector<std::string_view>{ argv, argv + argc };
    const auto has_option = [&args](std::string_view option) {
        return std::find(args.begin(), args.end(), option) != args.end();
    };
    if (has_option("-h") || has_option("--help")) {
        return print_help();
    }

    if (has_option("--version")) {
        return print_version();
    }

    if (has_option("--license")) {
        return print_license();
    }

    if (has_option("--help-tags")) {
        return print_tags();
    }

    const auto dir = args.size() > 1 ? args[1] : "."sv;
    const auto select = args.size() > 2 ? args[2] : ""sv;
    const auto explorer = glug::filesystem::explorer{
        dir, { glug::filter::select{ db.expand(select), dir } }
    };

    const auto trim_dot = dir == "." ? 2 : 0;
    for (const auto& file : explorer) {
        std::cout << file.path().generic_string().substr(trim_dot) << "\n";
    }
    return 0;
}

