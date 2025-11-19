# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
import os
import subprocess

import git

import pytest

import repos


PROJECT_ROOT = os.path.abspath(f'{os.path.dirname(__file__)}/../..')


def list_git(path: str) -> set[str]:
    tracked = set(git.ls_files(path)) - set(git.ls_tracked_ignored(path))
    return {file for file in tracked if not os.path.islink(f'{path}/{file}')}


def list_glug(path: str) -> set[str]:
    glug = f'{PROJECT_ROOT}/build/latest/glug'
    if not os.path.isfile(glug):
        glug += '.exe'
    return set(subprocess.check_output([glug], cwd=path).decode().splitlines())


@pytest.mark.parametrize(
    'repo,target',
    [
        (repos.LINUX, ''),
        (repos.DENO, ''),
        (repos.FASTAPI, ''),
        (repos.GODOT, ''),
        (repos.GODOT, 'editor'),
        (repos.GODOT, 'modules'),
        (repos.GODOT, 'tests/core/input'),
        (repos.TYPESCRIPT, ''),
        (repos.POWERSHELL, ''),
        (repos.VUEJS, ''),
    ],
    ids=str
)
def test_listing(repo: repos.Repo, target: str) -> None:
    data_dir = f'{PROJECT_ROOT}/test/data/.cloned'
    os.makedirs(data_dir, exist_ok=True)

    if repo.needs_symlinks and not repo.supports_symlinks(data_dir):
        pytest.skip('Skipping repo with symlinks')
    elif repo.needs_case_mix and not repo.supports_case_mix(data_dir):
        pytest.skip('Skipping repo with case-mixed files')

    path = git.clone_lean(repo.source, repo.branch, data_dir)
    target = f'{path}/{target}'
    assert list_glug(target) == list_git(target)
