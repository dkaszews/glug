# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Utilities for interacting with git repositories."""
import logging
import os
import re
import shutil
import subprocess
from typing import Self

from fasteners import InterProcessLock  # type: ignore


logging.basicConfig()


class Clone:
    """Represents cloned git repository."""

    def __init__(self, path: str) -> None:
        """Construct repository at given path."""
        self._cwd = path
        self._root: str | None = None

    @property
    def cwd(self) -> str:
        """Get working directory of the repository, passed at construction."""
        return self._cwd

    @property
    def root(self) -> str:
        """Get root of the repository, cached."""
        if self._root is None:
            self._root = self.cmd(['rev-parse', '--show-toplevel'])[0]
        return self._root

    @classmethod
    def clone_lean(cls, uri: str, branch: str, dest: str) -> Self:
        """
        Perform a "lean" clone of git repository.

        A "lean" clone recreates tree of files, but keeps most empty to save
        space. The only files that are checked out by default are `.gitignore`
        files and links to maintain the output of tools like `git ls-files`
        or `find`. The `.git` directory is replaced with an empty init to skip
        storing history.

        On large repositories this can be 50x smaller and is 2x faster.
        """
        name = re.search(r'/([^/.]+)\.git', uri)
        if not name:
            raise ValueError(f"Repo uri does not match '/().git': '{uri}'")

        clone_dir = os.path.abspath(f'{dest}/{name[1]}-{branch}')
        os.makedirs(dest, exist_ok=True)
        with InterProcessLock(f'{clone_dir}.lock'):
            if not os.path.isdir(clone_dir):
                _clone_lean_locked(uri, branch, clone_dir)
            return cls(clone_dir)

    def cmd(self, args: list[str], cwd: str | None = None) -> list[str]:
        """Run git command and return stdout as lines."""
        if cwd is None:
            cwd = self.cwd
        return self._cmd(args, cwd)

    def _join(self, subdir: str | None = None) -> str:
        return os.path.join(self.cwd, subdir or '')

    def get_tracked(
        self,
        subdir: str | None = None,
        abspath: bool = False
    ) -> list[str]:
        """List tracked files in repository."""
        return self._ls_cmd(['ls-files'], self._join(subdir), abspath)

    def get_tracked_ignored(
        self,
        subdir: str | None = None,
        abspath: bool = False
    ) -> list[str]:
        """List tracked files which are nonetheless matched by gitignore."""
        args = ['ls-files', '-ic', '--exclude-standard', '.']
        return self._ls_cmd(args, self._join(subdir), abspath)

    @classmethod
    def _cmd(cls, args: list[str], cwd: str) -> list[str]:
        args = ['git'] + args
        return subprocess.check_output(args, cwd=cwd).decode().splitlines()

    @classmethod
    def _ls_cmd(
        cls,
        args: list[str],
        cwd: str,
        abspath: bool = False
    ) -> list[str]:
        files = [cls.decode(path) for path in cls._cmd(args, cwd)]
        if not abspath:
            return files
        return [os.path.abspath(os.path.join(cwd, file)) for file in files]

    @classmethod
    def decode(cls, path: str) -> str:
        """Decode escaped path string."""
        if path[0] != '"':
            return path

        return (
            path[1:-1].encode('utf-8').decode('unicode_escape')
            .encode('latin1').decode('utf8')
        )


# TODO: Remove
def cmd(workdir: str, args: list[str]) -> list[str]:
    """Run git command and return stdout as lines."""
    return Clone(workdir).cmd(args)


# TODO: Remove
def ls_files(path: str, absolute: bool = False) -> list[str]:
    """List files tracked by git."""
    return Clone(path).get_tracked(abspath=absolute)


# TODO: Remove
def ls_tracked_ignored(path: str, absolute: bool = False) -> list[str]:
    """List files which have been ignored after committing or force-added."""
    return Clone(path).get_tracked_ignored(abspath=absolute)


def _parse_diff_index(line: str) -> tuple[str, bool]:
    (stat, path) = line.split('\t')
    is_symlink = stat[2] == '2'
    return (Clone.decode(path), is_symlink)


def _clone_lean_locked(uri: str, branch: str, dest: str) -> None:
    logging.info(f'Cloning {uri}:{branch} to {dest}')
    cmd('.', ['clone', uri, '--depth', '1', '-qnb', branch, dest])

    files = [
        _parse_diff_index(line)
        for line in cmd(dest, ['diff-index', branch])
    ]
    to_checkout = [
        path
        for (path, is_symlink) in files
        if is_symlink or os.path.basename(path) == '.gitignore'
    ]
    logging.info(f'Restoring {len(to_checkout)} files')
    cmd(dest, ['restore', '-s', branch] + to_checkout)

    to_touch = {path for (path, _) in files} - set(to_checkout)
    to_touch = {os.path.join(dest, path) for path in to_touch}
    logging.info(f'Creating {len(to_touch)} empty files')
    for file in to_touch:
        os.makedirs(os.path.dirname(file), exist_ok=True)
        with open(file, 'w'):
            pass

    logging.info('Removing git history')
    # Windows has tendency to lock random index files
    shutil.rmtree(f'{dest}/.git', ignore_errors=True)
    cmd(dest, ['init'])
    # Force-add ignored files which were tracked on remote.
    # This will not be picked up by glug, but maintains full repo shape.
    cmd(dest, ['add', '-f', '.'])
    cmd(dest, ['commit', '-m', f'Lean clone of {branch}'])
    logging.info('Done')
