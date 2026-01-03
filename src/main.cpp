// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#include "glug/filesystem.hpp"

#include <iostream>
#include <string_view>
#include <vector>

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

int main(int argc, const char** argv) {
    using namespace std::string_view_literals;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto args = std::vector<std::string_view>{ argv, argv + argc };
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

