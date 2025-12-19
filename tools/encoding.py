#!/usr/bin/env python3
# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Checks files are stored with correct encoding."""

import enum
import os
import sys

from git import Clone

from magic import Magic


class Newlines(enum.Enum):
    """Represents line endings in a file."""

    UNIX = enum.auto()
    DOS = enum.auto()
    MIXED = enum.auto()


class EncodingChecker:
    """Represents a file and its encoding."""

    _MAGIC = Magic(mime_encoding=True)

    def __init__(self, path: str):
        """Create EncodingChecker for file with given path."""
        self.path = path
        self.encoding = 'utf-8'
        self.newlines = Newlines.UNIX

        if not os.path.getsize(path):
            return

        self.encoding = self._MAGIC.from_file(path)
        match self.encoding:
            case 'binary':
                return
            case 'us-ascii':
                self.encoding = 'utf-8'

        with open(path, encoding=self.encoding) as file:
            _ = file.readlines()
            newlines = file.newlines

        match newlines:
            case None | '\n':
                self.newlines = Newlines.UNIX
            case '\r\n':
                self.newlines = Newlines.DOS
            case _:
                self.newlines = Newlines.MIXED

    def is_utf8(self) -> bool:
        """Check encoding is UTF8."""
        return self.encoding == 'utf-8'

    def is_unix(self) -> bool:
        """Check newlines are Unix (LF)."""
        return self.newlines == Newlines.UNIX

    def fix(self) -> None:
        """Reencode file as UTF-8 with Unix (LF) line endings."""
        with open(self.path, encoding=self.encoding) as file:
            lines = file.readlines()

        with open(self.path, 'w', newline='\n') as file:
            file.writelines(lines)


def main(path: str, fix: bool = False) -> bool:
    """Script entry."""
    if os.path.isdir(path):
        targets = Clone(path).get_tracked(abspath=True)
    else:
        targets = [path]

    encodings = [EncodingChecker(path) for path in targets]
    invalid = [
        encoding
        for encoding in encodings
        if not encoding.is_utf8() or not encoding.is_unix()
    ]
    if not invalid:
        print(f'Checked {len(targets)} files, all ok')
        return True

    out = sys.stdout if fix else sys.stderr
    for encoding in invalid:
        print(f'Invalid encoding for {encoding.path}, ', file=out)
        print('Expected: utf-8, newlines.UNIX', file=out)
        print(f'Actual:   {encoding.encoding}, {encoding.newlines}', file=out)

        if fix:
            encoding.fix()
            print('Fixed', file=out)
        else:
            print(f'Run `encoding.py --fix {encoding.path}`', file=out)

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
