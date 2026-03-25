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
        symlink_path = f'{path}/symlink.check'
        symlink_target = 'target.check'
        if os.path.islink(symlink_path):
            return True

        with open(f'{path}/{symlink_target}', 'w'):
            pass

        try:
            os.symlink(symlink_target, symlink_path)
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
        name = '/'.join(self.source.rsplit('/', maxsplit=2)[1:]).split('.')[0]
        return f'{name}:{self.branch}'


LINUX = Repo('https://github.com/torvalds/linux.git', 'v6.17', True, True)
DENO = Repo('https://github.com/denoland/deno.git', 'v2.5.6', True)
FASTAPI = Repo('https://github.com/fastapi/fastapi.git', '0.120.4')
GODOT = Repo('https://github.com/godotengine/godot.git', '4.5-stable')
TYPESCRIPT = Repo('https://github.com/microsoft/TypeScript.git', 'v5.9.3')
POWERSHELL = Repo('https://github.com/PowerShell/PowerShell.git', 'v7.5.4')
VUEJS = Repo('https://github.com/vuejs/core.git', 'v3.5.22')
MAGISK = Repo('https://github.com/topjohnwu/Magisk.git', 'v30.6', True)
JQ = Repo('https://github.com/jqlang/jq.git', 'jq-1.8.1', True, True)
OBS = Repo('https://github.com/obsproject/obs-studio.git', '32.0.4', True)
YCM = Repo(
    'https://github.com/ycm-core/YouCompleteMe.git',
    '79b7e5466564b17a1d4f36aa9e11e3fadc12ab1c'
)
