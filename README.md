# glug

Text search tool for programmers, inspired by [Ack Beyond Grep](https://github.com/beyondgrep/ack3) and [Ag Silver Searcher](https://github.com/ggreer/the_silver_searcher) and written in portable C++17.

## Philosophy

While `ack` and `ag` are written in Perl and C with Linux system headers respectively (port to Windows also exists), `glug` is written in fully portable C++17 instead, and aims to provide C bindings for use in other languages and programs (e.g. Python-based vim plugin).
It also strives for high quality of development, targetting 100% test coverage and 100% accurate `.gitignore` implementation[^1], all checked with CI workflows blocking any failing code from being committed.

[^1]: Parity tests ensure `glug` outputs the same files as `git ls-files`, save for non-file output (symlinks and submodules) and files matching `.gitignore` rules which have been accidentally committed.

## Usage

Currently, the provided binaries are for testing and demonstration purposes, showcasing the `.gitignore` implementation by recursively listing files in target directory.
`grep`-like in-file search is to be implemented by the next [0.2.0 release](https://github.com/dkaszews/glug/milestone/2).

Similarly to `ack` and `ag`, `glug` implements convenient shorthands for searching just in files related to given programming language.
Whereas those two spell it using separate options like `ag --cpp` or `ack --cpp --nohpp`, `glug` instead expands any glob prefixed with unescaped `#` into a set of globs stored in its database.
This allows unambiguous mixing of type tags with regular globs, so the two examples above would be spelled as `'#cpp'` and `'#cpp,-#hpp` respectively.

Currently implemented handful of type tags can be seen in `glug --help-tags`, with aim to expand in the future and allow user to extend it via config files.

## Installing

Check [releases tab](https://github.com/dkaszews/glug/releases) to find latest stable executable for your system.
Release binaries have format `glug-${version}-${os}-${arch}-${static:-}-${regex:-}`

`regex` is the regular expression provider, choosing alternative may increase your performance depending on your machine and usage:

1. `stl` - default `<regex>`, omitted in filename
1. [`pcre2`](https://github.com/PCRE2Project/pcre2) - popular implementation of Perl regex, usually already present on Linux as a common dependency
1. [`re2`](https://github.com/google/re2) - Google's implementation based on finite state machines
1. [`hyperscan`](https://github.com/intel/hyperscan) - Intel's x86-only implementation leveraging SIMD instructions to increase performance

### Compiling from source

1. Install [`xmake`](https://xmake.io/), can be found on most package providers such as `pacman`, `apt`, `winget`
1. Optionally, use `xmake config` to set compilation options (need to set them all at once, as each invocation might reset unmentioned options to default):
    * `--regex=stl|pcre2|re2|hyperscan`
    * `--toolchain=gcc|clang|msvc|...`
    * `--mode=release|releasedbg|debug|coverage`
    * `--kind=static|shared`
1. Run `xmake build -v` to compile executable and tests
1. Optionally, run tests with `xmake test -v`
    * `xmake test -v unit_test/default` runs regular [`gtest`-based](https://google.github.io/googletest/) UT
    * `xmake test -v parity_test/default` runs integration tests, comparing results of `glug` with those of `git ls-files` on popular repositories[^1]
1. Binaries can be found in `build/${os}/${arch}/${mode}`, copied to `build/latest`

