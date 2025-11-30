# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Utilities for interacting with git repositories."""
import dataclasses
import hashlib
import logging
import os
import re
import shutil
import subprocess
import timeit
import typing

from fasteners import InterProcessLock  # type: ignore


logging.basicConfig()


def cmd(workdir: str, args: list[str]) -> list[str]:
    """Run git command and return stdout as lines."""
    args = ['git'] + args
    return subprocess.check_output(args, cwd=workdir).decode().splitlines()


def _get_current_version() -> str:
    with open(__file__, 'rb') as file:
        return hashlib.sha256(file.read()).hexdigest()


def _check_version(dest: str) -> bool:
    version_file = f'{dest}.version'
    if os.path.isfile(version_file):
        with open(version_file) as file:
            clone_version = file.read()
    else:
        clone_version = ''

    if clone_version == _get_current_version():
        return True

    shutil.rmtree(dest, ignore_errors=True)
    shutil.rmtree(version_file, ignore_errors=True)
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


@dataclasses.dataclass(frozen=True)
class IndexItem:
    """Represents a file tracked by git with its attributes."""

    path: str
    is_symlink: bool

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
    _add_ignored_files(dest, files)
    _clean_history(dest, branch)
    logging.info(f'Done in {timeit.default_timer() - start:.1f}s')


def _checkout_or_touch(dest: str, branch: str, files: list[IndexItem]) -> None:
    to_checkout = [
        item.path
        for item in files
        if item.is_symlink or os.path.basename(item.path) == '.gitignore'
    ]
    logging.info(f'Restoring {len(to_checkout)} files')
    cmd(dest, ['restore', '-s', branch] + to_checkout)

    to_touch = {item.path for item in files} - set(to_checkout)
    to_touch = {os.path.join(dest, path) for path in to_touch}
    logging.info(f'Creating {len(to_touch)} empty files')
    for file in to_touch:
        os.makedirs(os.path.dirname(file), exist_ok=True)
        with open(file, 'w'):
            pass


def _add_ignored_files(dest: str, files: list[IndexItem]) -> None:
    logging.info(f'Adding ~{len(files)} ignored files')
    # TODO: Implement
    _ = (dest, files)


def _clean_history(dest: str, branch: str) -> None:
    logging.info('Removing git history')
    # Windows has tendency to lock random index files
    shutil.rmtree(f'{dest}/.git', ignore_errors=True)
    cmd(dest, ['init'])
    # Force-add ignored files which were tracked on remote.
    # This will not be picked up by glug, but maintains full repo shape.
    cmd(dest, ['add', '-f', '.'])
    cmd(dest, ['commit', '-m', f'Lean clone of {branch}'])
