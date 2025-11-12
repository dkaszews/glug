#!/usr/bin/env python3
# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Checks no tracked files match .gitignore rules."""

import sys

import git


def main(path: str) -> bool:
    """Script entry."""
    ignored = git.ls_tracked_ignored(path)
    if not ignored:
        print('No tracked files match .gitignore rules')
        return True

    for file in ignored:
        explain = git.cmd(path, ['check-ignore', '--no-index', '-v', file])[0]
        print(f'File {file} ignored by:\n{explain}', file=sys.stderr)

    return False


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        'path',
        nargs='?',
        default='.',
        help='Target directory, defaults to current dir'
    )
    sys.exit(0 if main(**vars(parser.parse_args())) else 1)
