# Parity tests

Parity tests are designed to check functionality which is expected to result in comparable if not exactly the same output to other tools running on large established codebases.
This is somewhat orthogonal to unit tests, which are test single functions or classes (units) against an artificial set of inputs and have explicit and exact expected outputs, parity tests aim to find edge cases in real-world scenarios that the unit tests did not consider.

## Examples

`glug::filesystem::explorer` is documented to provide exact same output as `git ls-files`.
This has an asterix in cases where an ignored file is tracked by the repo due to being committed before `.gitignore` has been updated, or has been committed with `git add --force`.
Since there is no way for `glug` to determine whether a file is actually tracked, such files have to be filtered out from the `git ls-files` output instead.

## Lean clones

To speed up cloning of test data and reduce storage requirements, a "lean" clone mechanism has been implemented.
It performs a clone without checking most of the repo, replacing everything except `.gitignore` files and symlinks with empty files and then deleting the history.
Result is a repository which looks normal according to `find` or `git ls-files`, but is up to 90-95% lighter, for example Linux kernel v6.17 goes from 2GB (shallow single branch) to 60MB.

## Running

Simply run `pytest -v test/parity` or `xmake test -v parity_test/default`, the later is recommended as it also makes sure that the `glug` executable is up-to-date.
It is recommended to use optimized binaries by default (`xmake config --mode=release` or `releasedbg`) as the debug ones can be order of magnitude slower (minutes vs seconds).

