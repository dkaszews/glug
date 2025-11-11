# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
import enum
import os
import subprocess

import git

import pytest


PROJECT_ROOT = os.path.abspath(f'{os.path.dirname(__file__)}/../..')
REPO_LINUX = 'https://github.com/torvalds/linux.git'


def list_git(path: str) -> set[str]:
    tracked = set(git.ls_files(path)) - set(git.ls_tracked_ignored(path))
    return {file for file in tracked if not os.path.islink(f'{path}/{file}')}


def list_glug(path: str) -> set[str]:
    glug = f'{PROJECT_ROOT}/build/latest/glug'
    if not os.path.isfile(glug):
        glug += '.exe'
    return set(subprocess.check_output([glug], cwd=path).decode().splitlines())


def supports_symlinks(path: str) -> bool:
    symlink_name = 'symlink.check'
    symlink_path = f'{path}/{symlink_name}'
    if os.path.islink(symlink_path):
        return True

    try:
        os.symlink(symlink_name, symlink_path)
        return True
    except OSError:
        return False


def supports_case_mix(path: str) -> bool:
    first = f'{path}/SpOnGeBoB.check'
    second = f'{path}/sPoNgEbOb.check'
    with open(first, 'w'), open(second, 'w'):
        return not os.path.samefile(first, second)


class Needs(enum.IntFlag):
    SYMLINKS = enum.auto()
    CASE_MIX = enum.auto()


@pytest.mark.parametrize(
    'repo,branch,needs',
    [
        (REPO_LINUX, 'v2.6.39', Needs.SYMLINKS | Needs.CASE_MIX),
        (REPO_LINUX, 'v3.19', Needs.SYMLINKS | Needs.CASE_MIX),
        (REPO_LINUX, 'v4.20', Needs.SYMLINKS | Needs.CASE_MIX),
        (REPO_LINUX, 'v5.19', Needs.SYMLINKS | Needs.CASE_MIX),
        (REPO_LINUX, 'v6.17', Needs.SYMLINKS | Needs.CASE_MIX),
        ('https://github.com/denoland/deno.git', 'v2.5.6', Needs.SYMLINKS),
        ('https://github.com/fastapi/fastapi.git', '0.120.4', Needs(0)),
        ('https://github.com/godotengine/godot.git', '4.5-stable', Needs(0)),
        ('https://github.com/microsoft/TypeScript.git', 'v5.9.3', Needs(0)),
        ('https://github.com/PowerShell/PowerShell.git', 'v7.5.4', Needs(0)),
        ('https://github.com/vuejs/core.git', 'v3.5.22', Needs(0)),
    ]
)
def test_listing(repo: str, branch: str, needs: Needs) -> None:
    # TODO: #35 Glug does not use .gitignore files in TypeScript repo
    if 'TypeScript' in repo:
        pytest.skip('Issue #35')

    data_dir = f'{PROJECT_ROOT}/test/data/.cloned'
    os.makedirs(data_dir, exist_ok=True)

    if needs & Needs.SYMLINKS and not supports_symlinks(data_dir):
        pytest.skip('Skipping repo with symlinks')
    elif needs & Needs.CASE_MIX and not supports_case_mix(data_dir):
        pytest.skip('Skipping repo with case-mixed files')

    path = git.clone_lean(repo, branch, data_dir)

    # TODO: #41 glug crashes on Windows on files with UTF8 name
    if 'deno' in repo and os.name == 'nt':
        os.remove(f'{path}/tests/unit_node/testdata/worker_module/βάρβαροι.js')
        git.cmd(path, ['add', '.'])

    assert list_glug(path) == list_git(path)
