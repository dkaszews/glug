// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#include "glug/filesystem.hpp"
#include "glug/regex.hpp"

#include "glug/generated/license.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

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

namespace {

constexpr auto help = std::string_view{ R"(
usage: glug [options] [root] [filter]

Search files respecting .gitignore and given filter.

Positional arguments:
  root         Search root, defaults to current directory
  filter       Comma-separated list of allowed globs and tags, defaults to all

Options:
  -h, --help   show this help message and exit
  --version    print glug version
  --license    print license of glug and third-party libraries, if any
  --help-tags  print builtin tag expansions

Examples:
  glug . '*.cpp'               # Search all '*.cpp' files
  glug include '-generated/*'  # Omit files in generated directory
  glug test '#cpp,-#hpp'       # Search all cpp-related files except headers
)" };

int print_help() {
    std::cout << help.substr(1);
    return 0;
}

int print_version() {
    std::cout << GLUG_VERSION << "\n";
    return 0;
}

int print_license() {
    std::cout << "--- glug license --- \n\n";
    std::cout << glug::generated::license::data << "\n";
    if (const auto license = glug::regex::engine::license(); !license.empty()) {
        std::cout << license << "\n";
    }
    return 0;
}

int print_tags() {
    const auto max_elem = std::ranges::max_element(
            tags, [](const auto& lhs, const auto& rhs) {
                return lhs.first.size() < rhs.first.size();
            }
    );
    const auto pad = max_elem->first.size() + 2;

    for (const auto& [tag, globs] : tags) {
        std::cout << tag << std::string(pad - tag.size(), ' ') << globs << "\n";
    }
    return 0;
}

}  // namespace

int main(int argc, const char** argv) {
    using namespace std::string_view_literals;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto args = std::vector<std::string_view>{ argv, argv + argc };
    const auto has_option = [&args](std::string_view option) {
        return std::ranges::find(args, option) != args.end();
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
    const auto db = glug::glob::typetag_database{ tags };
    const auto explorer = glug::filesystem::explorer{
        dir, { glug::filter::select{ db.expand(select), dir } }
    };

    const auto trim_dot = dir == "." ? 2 : 0;
    for (const auto& file : explorer) {
        std::cout << file.path().generic_string().substr(trim_dot) << "\n";
    }
    return 0;
}

