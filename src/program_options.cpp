// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#include "glug/program_options.hpp"

#include <utility>

namespace glug::program {

program_options program_options::parse(std::span<const std::string_view> args) {
    (void)args;
    return {};
}

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
    os << "flags";
    os << "{ .help = " << flags.help;
    os << ", .help_tags = " << flags.help_tags;
    os << ", .version = " << flags.version;
    os << ", .license = " << flags.license;
    return os << " }";
}

std::ostream& operator<<(std::ostream& os, const program_options& options) {
    os << "program_options";
    os << "{ .patterns = " << options.patterns;
    os << ", .paths = " << options.paths;
    os << ", .flags = " << options.flags;
    return os << " }";
}

}  // namespace glug::program


