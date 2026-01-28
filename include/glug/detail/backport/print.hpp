// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#pragma once

#include <version>

#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L

#include <print>

namespace glug::backport {

template <typename... ARGS>
void print(
        std::FILE* stream, std::format_string<ARGS...> format, ARGS&&... args
) {
    ::std::print(stream, format, std::forward<ARGS>(args)...);
}

template <typename... ARGS>
void print(std::format_string<ARGS...> format, ARGS&&... args) {
    ::std::print(format, std::forward<ARGS>(args)...);
}

template <typename... ARGS>
void println(
        std::FILE* stream, std::format_string<ARGS...> format, ARGS&&... args
) {
    ::std::println(stream, format, std::forward<ARGS>(args)...);
}

template <typename... ARGS>
void println(std::format_string<ARGS...> format, ARGS&&... args) {
    ::std::println(format, std::forward<ARGS>(args)...);
}

}  // namespace glug::backport

#else

#include <format>
#include <iostream>

// <print> header is still missing on clang and some gcc distributions
namespace glug::backport {

template <typename... ARGS>
void print(
        std::FILE* stream, std::format_string<ARGS...> format, ARGS&&... args
) {
    std::fputs(
            std::format(format, std::forward<ARGS>(args)...).c_str(), stream
    );
}

template <typename... ARGS>
void print(std::format_string<ARGS...> format, ARGS&&... args) {
    ::glug::backport::print(stdout, format, std::forward<ARGS>(args)...);
}

template <typename... ARGS>
void println(
        std::FILE* stream, std::format_string<ARGS...> format, ARGS&&... args
) {
    ::glug::backport::print(stream, format, std::forward<ARGS>(args)...);
    std::fputs("\n", stream);
}

template <typename... ARGS>
void println(std::format_string<ARGS...> format, ARGS&&... args) {
    ::glug::backport::println(stdout, format, std::forward<ARGS>(args)...);
}

}  // namespace glug::backport

#endif

