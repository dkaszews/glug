#!/usr/bin/env python3
# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
'''
License checker script, checks source files for up-to-date license header.
'''

from datetime import datetime
import logging
import os
import subprocess
import sys


def _list_directory(directory: str, recurse: bool = False) -> list[str]:
    if not recurse:
        return [
            entry
            for entry in os.listdir(directory)
            if os.path.isfile(entry)
        ]

    return [
        os.path.join(dirname, filename)
        for (dirname, _, filenames) in os.walk(directory)
        for filename in filenames
    ]


def _is_ignored(path: str) -> bool:
    if '/.git/' in os.path.realpath(path):
        return True

    result = subprocess.run(['git', 'check-ignore', path], capture_output=True)
    return result.returncode == 0


def _get_date_range(path: str) -> tuple[datetime, datetime]:
    args = ['git', 'log', '--follow', '--format=%ci', path]
    result = subprocess.run(args, capture_output=True)
    if not result.stdout:
        return (
            datetime.fromtimestamp(os.path.getctime(path)),
            datetime.fromtimestamp(os.path.getmtime(path)),
        )

    def parse_date(s: str) -> datetime:
        return datetime.fromisoformat(s)

    lines = result.stdout.decode().splitlines()
    return (
        datetime.fromisoformat(lines[0]),
        datetime.fromisoformat(lines[-1]),
    )


def _check_file(path: str, fix: bool = False) -> bool:
    comments = {
        '.cfg': None,
        '.clang-format': None,
        '.gitignore': None,
        '.json': None,
        '.md': None,
        '.yaml': None,
        'LICENSE': None,
        '.py': '#',
        '.lua': '--',
        '.cpp': '//',
        '.hpp': '//',
    }

    filetype = os.path.splitext(path)[1] or os.path.split(path)[1]
    if filetype not in comments:
        logging.error(f'Unexpected filetype or filetype: {path}')
        return False

    comment = comments[filetype]
    if comment is None:
        return True

    (created, modified) = _get_date_range(path)
    logging.debug(f'{path}: {created.date()} - {modified.date()}')

    f = 'Provided as part of glug under MIT license, (c) {} Dominik Kaszewski'
    (c, m) = (created.year, modified.year)
    expected = comment + ' ' + f.format(f'{c}-{m}' if c != m else f'{c}')

    with open(path) as file:
        lines = file.read().splitlines()

    has_shebang = lines[0].startswith('#!')
    actual = lines[has_shebang]
    has_license = f.split(',')[0] in actual
    logging.debug(f'has shebang: {has_shebang}, has license: {has_license}')

    if expected == actual:
        return True

    if not fix:
        logging.error(f'Copyright header missing or invalid: {path}')
        logging.error(f'Expected: {expected}')
        logging.error(f'Actual:   {actual}')
        return False

    if has_license:
        lines[has_shebang] = expected
    else:
        lines.insert(has_shebang, expected)

    with open(path, 'w') as file:
        file.write('\n'.join(lines) + '\n')

    return True


def run(targets: list[str], recurse: bool = False, fix: bool = False) -> bool:
    if missing := [target for target in targets if not os.path.exists(target)]:
        logging.error(f'Targets do not exist: {missing}')
        return False

    if directories := [target for target in targets if os.path.isdir(target)]:
        logging.debug(f'Directories: {directories}')
        targets = list(set(targets) - set(directories))
        targets += [
            file
            for directory in directories
            for file in _list_directory(directory, recurse)
            if not _is_ignored(file)
        ]
        logging.debug(f'Resolved targets: {targets}')

    # Don't short-circuit with `all` to print all errors at once
    return sum(_check_file(target, fix) for target in targets) == len(targets)


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        'targets',
        metavar='target',
        nargs='*',
        default=['.'],
        help='Target file(s) or directory(-ies), defaults to current dir'
    )
    parser.add_argument(
        '-r', '--recurse',
        action='store_true',
        help='recursely search target directory(-ies)'
    )
    parser.add_argument(
        '-f', '--fix',
        action='store_true',
        help='Automatically fix any errors'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Include verbose logs'
    )

    args = vars(parser.parse_args())
    verbose = args.pop('verbose')
    logging.basicConfig(
        format=(
            '%(filename)s:%(funcName)s:%(lineno)d: %(message)s'
            if verbose else '%(message)s'
        ),
        level=(logging.DEBUG if verbose else logging.INFO)
    )

    success = run(**args)
    sys.exit(0 if success else 1)
