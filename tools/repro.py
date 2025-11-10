#!/usr/bin/env python3
# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
"""nodoc."""
import os
import shutil
import subprocess

import git

# test/data/.cloned/linux-v2.6.39/arch/microblaze/configs
# test/data/.cloned/linux-v3.19/arch/arm/common
# test/data/.cloned/linux-v4.20/arch/csky/configs
# test/data/.cloned/linux-v5.19/scripts/dtc/libfdt
# test/data/.cloned/linux-v6.17/scripts/dtc/libfdt
# test/data/.cloned/PowerShell-v7.5.4/test/powershell/Modules/PowerShellGet


def minimize(glug: str, repo: str, dir_: str) -> None:
    """nodoc."""
    join = os.path.join
    for child in os.listdir(join(repo, dir_)):
        full = join(repo, dir_, child)
        if os.path.islink(full) or child == '.git':
            continue

        print(f'{join(dir_, child)}: ', end='')
        if not os.path.islink(full) and os.path.isdir(full):
            shutil.rmtree(full)
        else:
            os.remove(full)

        try:
            subprocess.check_output([glug], cwd=repo)
            git.cmd(repo, ['restore', join(dir_, child)])
            print('no repro')
        except subprocess.CalledProcessError as e:
            print(e.output.decode().splitlines()[-1])

        if os.path.isdir(full):
            minimize(glug, repo, join(dir_, child))


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument('glug')
    parser.add_argument('repo')
    parser.add_argument('-n', '--times', type=int, default=1)
    args = parser.parse_args()

    git.cmd(args.repo, ['restore', '.'])
    for _ in range(args.times):
        minimize(os.path.abspath(args.glug), args.repo, '')
