# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Utilities for interacting with git repositories."""
import dataclasses
import logging
import os
import re
import string
import subprocess
import timeit
import typing
from hashlib import sha256
from random import Random
from shutil import rmtree

from fasteners import InterProcessLock  # type: ignore


logging.basicConfig()


def cmd(workdir: str, args: list[str]) -> list[str]:
    """Run git command and return stdout as lines."""
    args = ['git'] + args
    return subprocess.check_output(args, cwd=workdir).decode().splitlines()


def _get_current_version() -> str:
    with open(__file__, 'rb') as file:
        return sha256(file.read()).hexdigest()


def _check_version(dest: str) -> bool:
    version_file = f'{dest}.version'
    if os.path.isfile(version_file):
        with open(version_file) as file:
            clone_version = file.read()
    else:
        clone_version = ''

    if clone_version == _get_current_version():
        return True

    rmtree(dest, ignore_errors=True)
    rmtree(version_file, ignore_errors=True)
    return False


def _write_version(dest: str) -> None:
    with open(f'{dest}.version', 'w') as file:
        file.write(_get_current_version())


def clone_lean(repo: str, branch: str, dest: str) -> str:
    """
    Perform a "lean" clone of git repository.

    A "lean" clone maintains a file tree, but keeps most files empty to save
    space. The only files that are checked out by default are `.gitignore`
    files and links to maintain the output of tools like `git ls-files`
    or `find`. The `.git` directory is replaced with an empty init to skip
    storing history.

    On large repositories this can be 50x smaller and is 2x faster.
    """
    name = re.search(r'/([^/.]+)\.git', repo)
    if not name:
        raise ValueError(f"Expected repo to match '/().git' but was '{repo}'")

    clone_dir = os.path.abspath(f'{dest}/{name[1]}-{branch}')
    os.makedirs(dest, exist_ok=True)
    with InterProcessLock(f'{clone_dir}.lock'):
        if not _check_version(clone_dir):
            _clone_lean_locked(clone_dir, repo, branch)
            _write_version(clone_dir)
        return clone_dir


def _resolve_absolute(root: str, files: list[str]) -> list[str]:
    return [os.path.abspath(f'{root}/{file}') for file in files]


def _decode_utf8_path(path: str) -> str:
    if path[0] != '"':
        return path

    return (
        path[1:-1].encode('utf-8').decode('unicode_escape')
        .encode('latin1').decode('utf8')
    )


def _ls_cmd(path: str, args: list[str], absolute: bool = False) -> list[str]:
    files = [_decode_utf8_path(path) for path in cmd(path, args)]
    return _resolve_absolute(path, files) if absolute else files


def ls_files(path: str, absolute: bool = False) -> list[str]:
    """List files tracked by git."""
    args = ['ls-files', '.']
    return _ls_cmd(path, args, absolute)


def ls_tracked_ignored(path: str, absolute: bool = False) -> list[str]:
    """List files which have been ignored after committing or force-added."""
    args = ['ls-files', '-ic', '--exclude-standard', '.']
    return _ls_cmd(path, args, absolute)


def ls_untracked(path: str, absolute: bool = False) -> list[str]:
    """List files which are neither tracked, nor ignored."""
    args = ['ls-files', '--others', '--exclude-standard']
    return _ls_cmd(path, args, absolute)


@dataclasses.dataclass(frozen=True)
class IndexItem:
    """Represents a file tracked by git with its attributes."""

    path: str
    is_symlink: bool

    @property
    def is_gitignore(self) -> bool:
        """Check if item is a .gitignore file."""
        return os.path.basename(self.path) == '.gitignore'

    @classmethod
    def parse_diff_line(cls, line: str) -> typing.Self:
        """Create IndexItem from line of `git diff-index`."""
        (stat, path) = line.split('\t')
        is_symlink = stat[2] == '2'
        return cls(_decode_utf8_path(path), is_symlink)


def _clone_lean_locked(dest: str, repo: str, branch: str) -> None:
    logging.info(f'Cloning {repo}:{branch} to {dest}')
    start = timeit.default_timer()
    cmd('.', ['clone', repo, '--depth', '1', '-qnb', branch, dest])

    files = [
        IndexItem.parse_diff_line(line)
        for line in cmd(dest, ['diff-index', branch])
    ]
    _checkout_or_touch(dest, branch, files)
    _add_ignored_files(dest, f'{repo}:{branch}', files)
    _clean_history(dest, branch)
    logging.info(f'Done in {timeit.default_timer() - start:.1f}s')


def _touch(path: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w'):
        pass


def _checkout_or_touch(dest: str, branch: str, files: list[IndexItem]) -> None:
    to_checkout = [
        item.path
        for item in files
        if item.is_symlink or item.is_gitignore
    ]
    logging.info(f'Restoring {len(to_checkout)} files')
    cmd(dest, ['restore', '-s', branch] + to_checkout)

    to_touch = {item.path for item in files} - set(to_checkout)
    to_touch = {os.path.join(dest, path) for path in to_touch}
    logging.info(f'Creating {len(to_touch)} empty files')
    for path in to_touch:
        _touch(path)


class GlobRandomizer:
    # `string.printable` contains control chars like vertical tab or backspace
    RETRIES = 10
    PRINTABLE_CHARS = set(c for c in string.printable if c.isprintable())
    BANNED_CHARS = set(r'<>:"/\|?*')
    ALLOWED_CHARS = PRINTABLE_CHARS - BANNED_CHARS
    BANNED_NAMES = {'.', '..', 'CON', 'PRN', 'AUX', 'NUL'}
    BANNED_NAMES |= {f'COM{i}' for i in range(10)}
    BANNED_NAMES |= {f'LPT{i}' for i in range(10)}

    def __init__(self, seed: str = '') -> None:
        self._rng = Random(seed)
        self._range_cache: dict[str, str] = {}

    def __call__(self, glob: str) -> str:
        return self.generate(glob)

    def generate(self, glob: str) -> str:
        assert glob
        glob = self._expand_anchors(glob)
        glob = self._expand_sets(glob)
        glob = self._expand_stars(glob)

        for _ in range(self.RETRIES):
            # TODO: double check with fnmatch?
            path = self._generate_once(glob)
            if self.is_valid_path(path):
                return path

        raise TimeoutError(f"Failed to generate path from glob '{glob}'")

    def _expand_anchors(self, glob: str) -> str:
        if '/' not in glob[:-1]:
            glob = f'**/{glob}'

        if glob[-1] == '/':
            glob = f'{glob}**/*'

        return glob

    def _expand_sets(self, glob: str) -> str:
        _ = glob
        raise NotImplementedError()

    def _expand_stars(self, glob: str) -> str:
        _ = glob
        raise NotImplementedError()

    def _generate_once(self, glob: str) -> str:
        _ = glob
        raise NotImplementedError()

    def is_valid_path(self, path: str) -> bool:
        if '/' in path:
            return all(self.is_valid_path(segment) for segment in path)

        if not path:
            return False
        if path[-1] in ' ':
            return False
        return path not in self.BANNED_NAMES


def _read_ignore_globs(path: str) -> list[str]:
    with open(path) as file:
        lines = [
            line.strip().lstrip('!/')
            for line in file.readlines()
        ]
    return [line for line in lines if line and line[0] != '#']


def _make_random_filename(rng: Random, globs: list[str]) -> str:
    glob = rng.choice(globs)
    # TODO: Implement
    return glob


def _add_ignored_files(dest: str, seed: str, files: list[IndexItem]) -> None:
    gitignores = [
        os.path.join(dest, item.path)
        for item in files
        if item.is_gitignore
    ]
    globs = {
        glob
        for path in gitignores
        for glob in _read_ignore_globs(path)
    }
    logging.info(
        f'Loaded {len(globs)} globs from {len(gitignores)} .gitignore files'
    )

    randomizer = GlobRandomizer(seed)
    generated = {
        path
        for glob in globs
        for path in randomizer(glob)
    }
    logging.info(f'Generated {len(generated)} random filenames')
    breakpoint()

    for name in generated:
        _touch(os.path.join(dest, name))

    untracked = ls_untracked(dest, True)
    logging.info(f'Removing {len(untracked)} false positives')
    for path in untracked:
        os.remove(path)


def _clean_history(dest: str, branch: str) -> None:
    logging.info('Removing git history')
    # Windows has tendency to lock random index files
    rmtree(f'{dest}/.git', ignore_errors=True)
    cmd(dest, ['init'])
    # Force-add ignored files which were tracked on remote.
    # This will not be picked up by glug, but maintains full repo shape.
    cmd(dest, ['add', '-f', '.'])
    cmd(dest, ['commit', '-m', f'Lean clone of {branch}'])
