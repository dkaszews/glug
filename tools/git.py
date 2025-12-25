# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Utilities for interacting with git repositories."""
import configparser
import dataclasses
import logging
import os
import re
import shutil
import subprocess
import time
from typing import Self

from fasteners import InterProcessLock  # type: ignore


logging.basicConfig()


def _git_path_decode(path: str) -> str:
    if path[0] != '"':
        return path

    return (
        path[1:-1].encode('utf-8').decode('unicode_escape')
        .encode('latin1').decode('utf8')
    )


@dataclasses.dataclass(frozen=True)
class Object:
    """Represents object tracked by git."""

    EXPECTED_MODES = ('100644', '100755', '120000', '160000')

    path: str
    mode: str
    hash: str

    @classmethod
    def parse(cls, line: str) -> Self:
        """
        Parse line coming from git diff-index.

        Examples ('r-' and 'l-' mean remote and local, hashes are truncated):
         r-mode l-mode r-hash   l-hash   m  path
        :100644 000000 8af04e10 00000000 D  .gitmodules
        :100644 100644 975a57b0 00000000 M  tools/git.py
        :160000 000000 7895484a 00000000 D  third_party/ycmd
        :120000 000000 fe0c45aa 00000000 D  tests/testdata/symlink_to_subdir
        """
        assert line[0] == ':'
        line = line[1:]
        (meta, path) = line.split('\t')
        (mode, _, hash, _, _) = meta.split(' ')
        assert mode in cls.EXPECTED_MODES
        return cls(_git_path_decode(path), mode, hash)

    @property
    def is_file(self) -> bool:
        """Check if object represents regular file."""
        return not self.is_symlink and not self.is_submodule

    @property
    def is_symlink(self) -> bool:
        """Check if object represents symlink."""
        return self.mode[1] == '2'

    @property
    def is_submodule(self) -> bool:
        """Check if object represents submodule."""
        return self.mode[1] == '6'

    @property
    def is_executable(self) -> bool:
        """Check if object represents executable file."""
        return self.mode[-1] == '5'


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
            if os.path.isdir(clone_dir):
                return cls(clone_dir)

            try:
                cls._clone_lean_locked(uri, branch, clone_dir)
                return cls(clone_dir)
            except Exception:
                shutil.rmtree(clone_dir, ignore_errors=True)
                raise

    @classmethod
    def _clone_lean_locked(cls, uri: str, branch: str, dest: str) -> None:
        # TODO: Break up into helpers
        started = time.time()
        logging.info(f'Cloning lean {uri}:{branch} to {dest}')
        # 40-char identifiers may be commit SHAs and cannot be cloned directly
        if len(branch) == 40:
            cls._cmd(['clone', uri, '--depth', '1', '-qn', dest])
            cls._cmd(['fetch', 'origin', branch], dest)
        else:
            cls._cmd(['clone', uri, '--depth', '1', '-qnb', branch, dest])

        objects = [
            Object.parse(line)
            for line in cls._cmd(['diff-index', branch], dest)
        ]

        checkout_names = ('.gitignore', '.gitmodules')
        to_checkout = {
            obj.path
            for obj in objects
            if obj.is_symlink or os.path.basename(obj.path) in checkout_names
        }
        logging.info(f'Restoring {len(to_checkout)} files')
        if to_checkout:
            cls._cmd(['restore', '-s', branch] + list(to_checkout), dest)

        submodules = {obj for obj in objects if obj.is_submodule}
        skip_touch = to_checkout | {obj.path for obj in submodules}
        to_touch = {
            os.path.join(dest, obj.path)
            for obj in objects
            if obj.path not in skip_touch
        }
        logging.info(f'Creating {len(to_touch)} empty files')
        for file in to_touch:
            os.makedirs(os.path.dirname(file), exist_ok=True)
            with open(file, 'w'):
                pass

        logging.info('Removing git history')
        # Windows has tendency to lock random index files
        shutil.rmtree(f'{dest}/.git', ignore_errors=True)
        cls._cmd(['init'], dest)
        # Force-add ignored files which were tracked on remote.
        # This will not be picked up by glug, but maintains full repo shape.
        cls._cmd(['add', '-f', '.'], dest)
        cls._cmd(['commit', '-m', f'Lean clone of {branch}'], dest)
        logging.info(f'Done in {time.time() - started:.3f}s')

        if not submodules:
            return

        logging.info(f'Recursing into {len(submodules)} submodules')
        config = configparser.ConfigParser(default_section='')
        config.read([os.path.join(dest, '.gitmodules')])
        urls = {
            section['path']: section['url']
            for (_, section) in config.items()
            if section
        }
        for submodule in submodules:
            path = os.path.join(dest, submodule.path)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            url = urls[submodule.path]
            cls._clone_lean_locked(url, submodule.hash, path)

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
    def _cmd(cls, args: list[str], cwd: str = '.') -> list[str]:
        args = ['git'] + args
        return subprocess.check_output(args, cwd=cwd).decode().splitlines()

    @classmethod
    def _ls_cmd(
        cls,
        args: list[str],
        cwd: str,
        abspath: bool = False
    ) -> list[str]:
        files = [_git_path_decode(path) for path in cls._cmd(args, cwd)]
        if not abspath:
            return files
        return [os.path.abspath(os.path.join(cwd, file)) for file in files]
