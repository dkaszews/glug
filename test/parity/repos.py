# Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
import dataclasses
import os


@dataclasses.dataclass(frozen=True)
class Repo:
    source: str
    branch: str
    needs_symlinks: bool = False
    needs_case_mix: bool = False

    @classmethod
    def supports_symlinks(cls, path: str) -> bool:
        symlink_name = 'symlink.check'
        symlink_path = f'{path}/{symlink_name}'
        if os.path.islink(symlink_path):
            return True

        try:
            os.symlink(symlink_name, symlink_path)
            return True
        except OSError:
            return False

    @classmethod
    def supports_case_mix(cls, path: str) -> bool:
        first = f'{path}/SpOnGeBoB.check'
        second = f'{path}/sPoNgEbOb.check'
        with open(first, 'w'), open(second, 'w'):
            return not os.path.samefile(first, second)

    def __str__(self) -> str:
        return f'{self.source}:{self.branch}'


LINUX_2 = Repo('https://github.com/torvalds/linux.git', 'v2.6.39', True, True)
LINUX_3 = Repo('https://github.com/torvalds/linux.git', 'v3.19', True, True)
LINUX_4 = Repo('https://github.com/torvalds/linux.git', 'v4.20', True, True)
LINUX_5 = Repo('https://github.com/torvalds/linux.git', 'v5.19', True, True)
LINUX_6 = Repo('https://github.com/torvalds/linux.git', 'v6.17', True, True)
DENO = Repo('https://github.com/denoland/deno.git', 'v2.5.6', True)
FASTAPI = Repo('https://github.com/fastapi/fastapi.git', '0.120.4')
GODOT = Repo('https://github.com/godotengine/godot.git', '4.5-stable')
TYPESCRIPT = Repo('https://github.com/microsoft/TypeScript.git', 'v5.9.3')
POWERSHELL = Repo('https://github.com/PowerShell/PowerShell.git', 'v7.5.4')
VUEJS = Repo('https://github.com/vuejs/core.git', 'v3.5.22')
