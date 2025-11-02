#!/usr/bin/env python3
# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""Utilities for interacting with git repositories."""
import logging
import os
import re
import shutil
import subprocess

from fasteners import InterProcessLock  # type: ignore


logging.basicConfig()


def cmd(workdir: str, args: list[str]) -> list[str]:
    """Run git command and return stdout as lines."""
    args = ['git'] + args
    return subprocess.check_output(args, cwd=workdir).decode().splitlines()


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
        if not os.path.isdir(clone_dir):
            _clone_lean_locked(repo, branch, clone_dir)
        return clone_dir


def ls_files(path: str) -> list[str]:
    """List files tracked by git."""
    return cmd(path, ['ls-files', '.'])


def ls_tracked_ignored(path: str) -> list[str]:
    """List files which have been ignored after committing or force-added."""
    return cmd(path, ['ls-files', '-ic', '--exclude-standard', '.'])


def _parse_diff_index(line: str) -> tuple[str, bool]:
    (stat, path) = line.split('\t')
    is_symlink = stat[2] == '2'
    if path[0] == '"' or path[-1] == '"':
        path = (
            path[1:-1].encode('utf-8').decode('unicode_escape')
            .encode('latin1').decode('utf8')
        )
    return (path, is_symlink)


def _clone_lean_locked(repo: str, branch: str, dest: str) -> None:
    logging.info(f'Cloning {repo}:{branch} to {dest}')
    cmd('.', ['clone', repo, '--depth', '1', '-qnb', branch, dest])

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
