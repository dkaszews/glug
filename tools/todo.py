#!/usr/bin/env python3
# Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
"""
Checks source files for untagged TODO comments.

$GH_TOKEN should provided for authentication, otherwise might silently
fail due to rate limiting.
"""

import os
import re
import sys

from git import Clone

import github


REPO = 'dkaszews/glug'


def get_allowed_comments() -> set[str]:
    """Get list of open issues for repo with their titles."""
    token = os.environ.get('GH_TOKEN', None)
    auth = github.Auth.Token(token) if token else None
    return {
        f'TODO: #{issue.number} - {issue.title}'
        for issue in github.Github(auth=auth).get_repo(REPO).get_issues()
    }


# TODO: Common code between all tools
class TodoChecker:
    """Represents file with all its TODO comments."""

    REGEX = re.compile(
        r'(?://|/\*|#|--)\s*((?:todo|fixme|bugbug)\b.*$)',
        re.IGNORECASE
    )

    def __init__(self, path: str) -> None:
        """Create TodoChecker for file with given path."""
        with open(path) as file:
            self._lines = file.read().splitlines()

        self.comments = [
            # TODO: dataclass
            (path, i, match.groups()[0])
            for (i, line) in enumerate(self._lines)
            if (match := self.REGEX.search(line))
        ]


# TODO: Common code between all tools
def main(path: str, fix: bool = False) -> bool:
    """Script entry."""
    if fix:
        raise NotImplementedError('Autofix not available')

    if os.path.isdir(path):
        targets = Clone(path).get_tracked(abspath=True)
    else:
        targets = [path]

    allowed_comments = get_allowed_comments()
    comments = [
        (path, line, comment)
        for target in targets
        for (path, line, comment) in TodoChecker(target).comments
    ]
    invalid = [
        comment
        for comment in comments
        if comment[2] not in allowed_comments
    ]
    if not invalid:
        print(f'Checked {len(targets)} files, {len(comments)}, all ok')
        return True

    out = sys.stderr
    print('Allowed TODO comments based on open issues:', file=out)
    print('\n'.join(allowed_comments), file=out)
    print(file=out)

    for (path, line, comment) in invalid:
        print(f'Invalid comment: {path}:{line}: {comment}', file=out)

    return False


# TODO: Common code between all tools
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
