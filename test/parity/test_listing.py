# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
import glob
import os
import subprocess

import git

import pytest


PROJECT_ROOT = os.path.abspath(f'{os.path.dirname(__file__)}/../..')


def list_git(path: str) -> set[str]:
    return set(git.ls_files(path)) - set(git.ls_tracked_ignored(path))


def list_glug(path: str) -> set[str]:
    glug = os.environ.get('GLUG_PARITY_EXE', None)
    if not glug:
        exe = 'glug.exe' if os.name == 'nt' else 'glug'
        glug = sorted(
            glob.glob(f'{PROJECT_ROOT}/build/*/*/*/{exe}'),
            key=os.path.getmtime
        )[-1]
    return set(subprocess.check_output([glug], cwd=path).decode().splitlines())


@pytest.mark.parametrize(
    'repo,branch',
    [
        ('https://github.com/torvalds/linux.git', 'v2.6.39'),
        # ('https://github.com/torvalds/linux.git', 'v3.19'),
        # ('https://github.com/torvalds/linux.git', 'v4.20'),
        # ('https://github.com/torvalds/linux.git', 'v5.19'),
        # ('https://github.com/torvalds/linux.git', 'v6.17'),
    ]
)
def test_listing(repo: str, branch: str) -> None:
    data_dir = f'{PROJECT_ROOT}/test/data/.cloned'
    path = git.clone_lean(repo, branch, data_dir)
    assert list_glug(path) == list_git(path)
