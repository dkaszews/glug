#pragma once

#include <format>
#include <iostream>

// <print> header is still missing on clang and some gcc distributions
namespace glug::backport {

template <typename... ARGS>
void print(
        std::FILE* stream, std::format_string<ARGS...> format, ARGS&&... args
) {
    fprintf(stream,
            "%s",
            std::format(format, std::forward<ARGS>(args)...).c_str());
}

template <typename... ARGS>
void print(std::format_string<ARGS...> format, ARGS&&... args) {
    print(stdout, format, std::forward<ARGS>(args)...);
}

template <typename... ARGS>
void println(
        std::FILE* stream, std::format_string<ARGS...> format, ARGS&&... args
) {
    fprintf(stream,
            "%s\n",
            std::format(format, std::forward<ARGS>(args)...).c_str());
}

template <typename... ARGS>
void println(std::format_string<ARGS...> format, ARGS&&... args) {
    println(stdout, format, std::forward<ARGS>(args)...);
}

}  // namespace glug::backport

