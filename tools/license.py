#!/usr/bin/env python3
# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Checks source files for up-to-date license header."""

import os
import re
import sys
from datetime import datetime

from git import Clone


class LicenseChecker:
    """Represents a file with its license header."""

    PREAMBLE = 'Provided as part of glug under MIT license, (c)'
    AUTHOR = 'Dominik Kaszewski'
    REGEX = re.compile(
        re.escape(PREAMBLE)
        + ' (?P<from>\\d{4})(?:-(?P<to>\\d{4}))? '
        + re.escape(AUTHOR)
    )
    FILETYPES = {
        '.cfg': None,
        '.clang-format': None,
        '.gitignore': None,
        '.json': None,
        '.md': None,
        '.yaml': None,
        '.py': '#',
        '.lua': '--',
        '.cpp': '//',
        '.hpp': '//',
    }

    def __init__(self, path: str):
        """Create LicenseChecker for file with given path."""
        self.path = path
        with open(path) as file:
            self._lines = file.read().splitlines()

        (self.commit_from, self.commit_to) = self._get_commit_history(path)
        self.license_line = self._get_license(self._lines)
        (self.license_from, self.license_to) = (
            self._get_license_history(self.license_line)
        )

    def expected_license(self) -> str:
        """Generate expected license header with correct year range."""
        from_ = self.commit_from.year
        to = self.commit_to.year
        if self.license_from and self.license_from.year < from_:
            from_ = self.license_from.year
        from_to = f'{from_}-{to}' if from_ != to else f'{from_}'

        return (
            f'{self.get_filetype(self.path)} '
            f'{self.PREAMBLE} {from_to} {self.AUTHOR}'
        )

    def fix(self) -> None:
        """Prepend expected license to file or replace existing one."""
        lines = self._lines[:]
        has_shebang = lines[0].startswith('#!')
        if self.license_line is not None:
            lines[has_shebang] = self.expected_license()
        else:
            lines.insert(has_shebang, self.expected_license())

        with open(self.path, 'w') as file:
            for line in lines:
                file.write(f'{line}\n')

    @classmethod
    def get_filetype(cls, path: str) -> str | None:
        """Get filetype for file, i.e. extension or entire filename if none."""
        return cls.FILETYPES.get(
            os.path.splitext(path)[1] or os.path.split(path)[1],
            None
        )

    @classmethod
    def is_licenseable(cls, path: str) -> bool:
        """
        Check if file should contain license.

        @return `True` for sources, `False` for configs and test data.
        """
        return cls.get_filetype(path) is not None

    @classmethod
    def _get_commit_history(cls, path: str) -> (
        tuple[datetime, datetime]
    ):
        args = ['log', '--follow', '--format=%ci', path]
        logs = Clone(os.path.dirname(path)).cmd(args)
        if not logs:
            return (
                datetime.fromtimestamp(os.path.getctime(path)),
                datetime.fromtimestamp(os.path.getmtime(path)),
            )

        return (
            datetime.fromisoformat(logs[-1]),
            datetime.fromisoformat(logs[0]),
        )

    @classmethod
    def _get_license(cls, lines: list[str]) -> str | None:
        if not lines:
            return None

        has_shebang = lines[0].startswith('#!')
        if has_shebang and len(lines) < 2:
            return None

        license_line = lines[has_shebang]
        return license_line if cls.REGEX.search(license_line) else None

    @classmethod
    def _get_license_history(cls, license_line: str | None) -> (
        tuple[datetime | None, datetime | None]
    ):
        none = (None, None)
        if not license_line:
            return none

        if match := cls.REGEX.search(license_line):
            return (
                datetime.strptime(match['from'], '%Y'),
                datetime.strptime(match['to'] or match['from'], '%Y')
            )

        return none


def main(path: str, fix: bool = False) -> bool:
    """Script entry."""
    if os.path.isdir(path):
        targets = Clone(path).get_tracked(abspath=True)
    else:
        targets = [path]

    licenses = [
        LicenseChecker(target)
        for target in targets
        if LicenseChecker.is_licenseable(target)
    ]
    invalid = [
        license_
        for license_ in licenses
        if license_.license_line != license_.expected_license()
    ]
    if not invalid:
        print(f'Checked {len(targets)} files, {len(licenses)} licenseable')
        return True

    output = sys.stdout if fix else sys.stderr
    for license_ in invalid:
        print(f'Invalid license for {license_.path}, ', file=output)
        print(f'Expected: {license_.expected_license()}', file=output)
        print(f'Actual:   {license_.license_line}', file=output)
        if fix:
            license_.fix()
            print('Fixed', file=output)
        else:
            print(f'Run `license.py --fix {license_.path}`', file=output)

    return fix


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        'path',
        nargs='?',
        default='.',
        help='Target file or directory, defaults to current dir'
    )
    parser.add_argument(
        '-f', '--fix',
        action='store_true',
        help='Automatically fix any errors'
    )

    sys.exit(0 if main(**vars(parser.parse_args())) else 1)
