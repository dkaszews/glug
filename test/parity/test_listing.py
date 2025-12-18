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
        # TODO: Commented temporarily to speed up testing
        # (repos.LINUX, ''),
        # (repos.DENO, 'tests'),
        # (repos.DENO, 'tests/specs/fmt'),
        # (repos.FASTAPI, ''),
        # (repos.FASTAPI, 'scripts'),
        (repos.GODOT, ''),
        # (repos.GODOT, 'editor'),
        # (repos.GODOT, 'modules/mono/editor'),
        # (repos.GODOT, 'platform/android'),
        # (repos.TYPESCRIPT, ''),
        # (repos.POWERSHELL, 'tools'),
        # (repos.POWERSHELL, 'tools/packaging'),
        # (repos.POWERSHELL, 'src'),
        # (repos.VUEJS, ''),
    ],
    ids=str
)
def test_listing_repo(repo: repos.Repo, target: str) -> None:
    data_dir = f'{PROJECT_ROOT}/test/data/.cloned'
    os.makedirs(data_dir, exist_ok=True)

    if repo.needs_symlinks and not repo.supports_symlinks(data_dir):
        pytest.skip('Skipping repo with symlinks')
    elif repo.needs_case_mix and not repo.supports_case_mix(data_dir):
        pytest.skip('Skipping repo with case-mixed files')

    path = f'{git.clone_lean(repo.source, repo.branch, data_dir)}/{target}'
    assert list_glug(path) == list_git(path)


@pytest.mark.parametrize(
    'target',
    [
        '',
        'src',
        'include',
        'include/glug',
        'test',
        'test/parity',
        'test/unit',
    ],
    ids=str
)
def test_listing_self(target: str) -> None:
    path = f'{PROJECT_ROOT}/{target}'
    assert list_glug(path) == list_git(path)
