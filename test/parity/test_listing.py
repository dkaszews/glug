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
        ('https://github.com/torvalds/linux.git', 'v3.19'),
        ('https://github.com/torvalds/linux.git', 'v4.20'),
        ('https://github.com/torvalds/linux.git', 'v5.19'),
        ('https://github.com/torvalds/linux.git', 'v6.17'),
        ('https://github.com/denoland/deno.git', 'v2.5.6'),
        ('https://github.com/fastapi/fastapi.git', '0.120.4'),
        ('https://github.com/godotengine/godot.git', '4.5-stable'),
        ('https://github.com/microsoft/TypeScript.git', 'v5.9.3'),
        ('https://github.com/PowerShell/PowerShell.git', 'v7.5.4'),
        ('https://github.com/vuejs/core.git', 'v3.5.22'),
    ]
)
def test_listing(repo: str, branch: str) -> None:
    if 'linux' in repo and os.name == 'nt':
        # Linux repo does not like Windows, requiring case-sensitive fs
        # and symlinks, both of which are disabled by default.
        pytest.skip('Skipping Linux repo on Windows')

    data_dir = f'{PROJECT_ROOT}/test/data/.cloned'
    path = git.clone_lean(repo, branch, data_dir)
    assert list_glug(path) == list_git(path)
