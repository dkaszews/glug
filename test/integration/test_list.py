# Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
import os
import subprocess

import git

import pytest

import repos


PROJECT_ROOT = os.path.abspath(f'{os.path.dirname(__file__)}/../..')


def list_git(clone: git.Clone, subdir: str | None = None) -> set[str]:
    files = set(clone.get_tracked(subdir))
    files -= set(clone.get_tracked_ignored(subdir))

    def is_link_or_submodule(file: str) -> bool:
        full = os.path.join(clone.cwd, subdir or '', file)
        return os.path.islink(full) or os.path.isdir(full)

    return {
        file
        for file in files
        if not is_link_or_submodule(file)
    }


def list_glug(
    clone: git.Clone,
    subdir: str | None = None,
    filter: str | None = None
) -> set[str]:
    args = [f'{PROJECT_ROOT}/build/latest/glug', '-E']
    if not os.path.isfile(args[0]):
        args[0] += '.exe'

    if filter is not None:
        args += ['-f', filter]

    path = f'{clone.cwd}/{subdir}'
    return set(subprocess.check_output(args, cwd=path).decode().splitlines())


@pytest.mark.parametrize(
    'repo,subdir',
    [
        (repos.DENO, ''),
        (repos.DENO, 'tests'),
        (repos.DENO, 'tests/specs/fmt'),
        (repos.FASTAPI, ''),
        (repos.FASTAPI, 'scripts'),
        (repos.GODOT, ''),
        (repos.GODOT, 'editor'),
        (repos.GODOT, 'modules/mono/editor'),
        (repos.GODOT, 'platform/android'),
        (repos.JQ, ''),
        (repos.JQ, 'config/m4'),
        (repos.JQ, 'tests/torture'),
        (repos.JQ, 'vendor/oniguruma'),
        (repos.JQ, 'vendor/oniguruma/harnesses'),
        (repos.LINUX, ''),
        (repos.LINUX, 'rust/kernel/alloc'),
        (repos.LINUX, 'drivers/gpu/drm/amd'),
        (repos.MAGISK, ''),
        (repos.MAGISK, 'app/test/src/main'),
        (repos.MAGISK, 'native/src/boot'),
        (repos.MAGISK, 'native/src/external/lz4/programs'),
        (repos.OBS, ''),
        (repos.OBS, 'plugins/obs-websocket/docs/comments'),
        (repos.OBS, 'shared/qt'),
        (repos.POWERSHELL, ''),
        (repos.POWERSHELL, 'src'),
        (repos.POWERSHELL, 'tools'),
        (repos.POWERSHELL, 'tools/packaging'),
        (repos.TYPESCRIPT, ''),
        (repos.TYPESCRIPT, 'src/jsTyping'),
        (repos.VUEJS, ''),
        (repos.VUEJS, 'scripts'),
        (repos.YCM, ''),
        (repos.YCM, 'test'),
    ],
    ids=str
)
def test_list_repo(repo: repos.Repo, subdir: str) -> None:
    data_dir = f'{PROJECT_ROOT}/test/data/.cloned'
    os.makedirs(data_dir, exist_ok=True)

    if repo.needs_symlinks and not repo.supports_symlinks(data_dir):
        pytest.skip('Skipping repo with symlinks')
    elif repo.needs_case_mix and not repo.supports_case_mix(data_dir):
        pytest.skip('Skipping repo with case-mixed files')

    clone = git.Clone.clone_lean(repo.source, repo.branch, data_dir)
    assert list_glug(clone, subdir) == list_git(clone, subdir)


@pytest.mark.parametrize(
    'subdir',
    [
        '',
        'src',
        'include',
        'include/glug',
        'test',
        'test/integration',
        'test/unit',
    ],
    ids=str
)
def test_list_self(subdir: str) -> None:
    clone = git.Clone(PROJECT_ROOT)
    assert list_glug(clone, subdir) == list_git(clone, subdir)


@pytest.mark.parametrize(
    'subdir,filter,expected',
    [
        (
            'test/data/hello_cpp',
            '*.hpp',
            {
                'include/hello_cpp/hello.hpp',
            },
        ),
        (
            'test/data/hello_cpp',
            '*.cpp',
            {
                'src/hello.cpp',
                'src/main.cpp',
                'test/main.cpp',
                'test/test_hello.cpp',
            },
        ),
        (
            'test/data/hello_cpp',
            '*.cpp,-test/',
            {
                'src/hello.cpp',
                'src/main.cpp',
            },
        ),
    ],
    ids=str
)
def test_list_filter(subdir: str, filter: str, expected: set[str]) -> None:
    clone = git.Clone(PROJECT_ROOT)
    assert list_glug(clone, subdir, filter=filter) == expected
