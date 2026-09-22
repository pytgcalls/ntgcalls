#!/usr/bin/env python3
"""Run clang-tidy on the staged sources, using an existing build directory."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CANDIDATES = (
    'build',
    'cmake-build-debug',
    'cmake-build-release',
)


def find_compile_commands() -> Path | None:
    configured = os.environ.get('NTGCALLS_COMPILE_COMMANDS')
    if configured:
        path = Path(configured)
        return path if path.is_file() else None
    for candidate in CANDIDATES:
        path = ROOT / candidate / 'compile_commands.json'
        if path.is_file():
            return path
    return next(ROOT.glob('build/temp.*/ntgcalls/compile_commands.json'), None)


def known_sources(compile_commands: Path) -> set[Path]:
    with compile_commands.open(encoding='utf-8') as handle:
        entries = json.load(handle)
    return {Path(entry['file']).resolve() for entry in entries}


def main() -> int:
    clang_tidy = os.environ.get('CLANG_TIDY') or shutil.which('clang-tidy')
    if clang_tidy is None:
        print('clang-tidy not found, skipping')
        return 0
    compile_commands = find_compile_commands()
    if compile_commands is None:
        print('No compile_commands.json found, skipping')
        return 0
    sources = known_sources(compile_commands)
    targets = [
        path
        for path in (Path(arg).resolve() for arg in sys.argv[1:])
        if path in sources
    ]
    if not targets:
        return 0
    command = [
        clang_tidy,
        '-p',
        str(compile_commands.parent),
        '--quiet',
        '--warnings-as-errors=*',
        *(str(target) for target in targets),
    ]
    return subprocess.run(command, cwd=ROOT, check=False).returncode


if __name__ == '__main__':
    sys.exit(main())
