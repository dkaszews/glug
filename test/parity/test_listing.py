# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
import os
import subprocess

import git

import pytest

import repos


PROJECT_ROOT = os.path.abspath(f'{os.path.dirname(__file__)}/../..')


def list_git(clone: git.Clone, subdir: str | None = None) -> set[str]:
    files = set(clone.get_tracked(subdir))
    files -= set(clone.get_tracked_ignored(subdir))
    return {
        file
        for file in files
        if not os.path.islink(os.path.join(clone.cwd, subdir or '', file))
    }


def list_glug(clone: git.Clone, subdir: str | None = None) -> set[str]:
    glug = f'{PROJECT_ROOT}/build/latest/glug'
    path = f'{clone.cwd}/{subdir}'
    if not os.path.isfile(glug):
        glug += '.exe'
    return set(subprocess.check_output([glug], cwd=path).decode().splitlines())


@pytest.mark.parametrize(
    'repo,subdir',
    [
        (repos.LINUX, ''),
        (repos.DENO, ''),
        (repos.DENO, 'tests'),
        (repos.DENO, 'tests/specs/fmt'),
        (repos.FASTAPI, ''),
        (repos.FASTAPI, 'scripts'),
        (repos.GODOT, ''),
        (repos.GODOT, 'editor'),
        (repos.GODOT, 'modules/mono/editor'),
        (repos.GODOT, 'platform/android'),
        (repos.TYPESCRIPT, ''),
        (repos.POWERSHELL, 'tools'),
        (repos.POWERSHELL, 'tools/packaging'),
        (repos.POWERSHELL, 'src'),
        # TODO: Add subdirs
        (repos.VUEJS, ''),
        (repos.MAGISK, ''),
        (repos.JQ, ''),
        (repos.OBS, ''),
    ],
    ids=str
)
def test_listing_repo(repo: repos.Repo, subdir: str) -> None:
    data_dir = f'{PROJECT_ROOT}/test/data/.cloned'
    os.makedirs(data_dir, exist_ok=True)

    if repo.needs_symlinks and not repo.supports_symlinks(data_dir):
        pytest.skip('Skipping repo with symlinks')
    elif repo.needs_case_mix and not repo.supports_case_mix(data_dir):
        pytest.skip('Skipping repo with case-mixed files')

    clone = git.Clone.clone_lean(repo.source, repo.branch, data_dir)
    if os.path.isfile(os.path.join(clone.cwd, '.gitmodules')):
        pytest.skip('Issue: #123')  # TODO: Add issue number
    assert list_glug(clone, subdir) == list_git(clone, subdir)


@pytest.mark.parametrize(
    'subdir',
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
def test_listing_self(subdir: str) -> None:
    clone = git.Clone(PROJECT_ROOT)
    assert list_glug(clone, subdir) == list_git(clone, subdir)
