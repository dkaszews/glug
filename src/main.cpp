// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filesystem.hpp"

#include <iostream>
#include <string_view>
#include <vector>

int main(int argc, const char** argv) {
    using namespace std::string_view_literals;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto args = std::vector<std::string_view>{ argv, argv + argc };
    const auto dir = args.size() > 1 ? args[1] : "."sv;
    const auto trim_dot = dir == "." ? 2 : 0;
    const auto explorer = glug::filesystem::explorer{ dir };
 
    for (const auto& file : explorer) {
        std::cout << file.path().generic_string().substr(trim_dot) << "\n";
    }
    return 0;
}

