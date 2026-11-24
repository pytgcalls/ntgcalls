import argparse
import io
import os
import re
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path

RELEASE_URL = 'https://github.com/pytgcalls/ntgcalls/releases/download/v{version}/ntgcalls.{platform}_symbols.zip'
CACHE_DIR = Path.home() / '.cache' / 'ntgcalls' / 'symbols'
FRAME = re.compile(r'^#(\d+)\s+(0x[0-9a-fA-F]+) in (.+?) from (\S+)\s*$')
LIBRARY = re.compile(r'^(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(?:Yes|No)(?:\s+\(\*\))?\s+(\S+)\s*$')
PYTHON_MODULE = re.compile(r'^ntgcalls\.cpython-\d+t?-(x86_64|aarch64)-linux-(gnu|musl)\.so$')
SOURCE_PATH = re.compile(r'.*?/((?:ntgcalls|wrtc)/(?:src|include)/.*|deps/.*)')
ARCHS = {'x86_64': 'x86_64', 'aarch64': 'arm64'}


def library_target(name: str) -> str | None:
    if PYTHON_MODULE.match(name):
        return 'python'
    if name == 'ntgcalls.node':
        return 'node'
    if name.startswith('libntgcalls.so'):
        return 'c'
    return None


def library_platform(name: str) -> str | None:
    match = PYTHON_MODULE.match(name)
    if not match:
        return None
    arch, libc = match.groups()
    return f'{"muslinux" if libc == "musl" else "linux"}-{ARCHS[arch]}'


def parse_backtrace(text: str) -> tuple[list[str], dict[str, int]]:
    lines = text.splitlines()
    bases = {}
    for line in lines:
        match = LIBRARY.match(line.strip())
        if match:
            bases[os.path.basename(match.group(3))] = int(match.group(1), 16)
    return lines, bases


def symbols_folder(version: str, platform: str, source: str | None) -> Path:
    if source:
        path = Path(source)
        if path.suffix == '.zip':
            target = CACHE_DIR / 'local' / path.stem
            with zipfile.ZipFile(path) as archive:
                archive.extractall(target)
            return target
        return path
    target = CACHE_DIR / version / platform
    if not target.exists():
        url = RELEASE_URL.format(version=version, platform=platform)
        print(f'Downloading {url}', file=sys.stderr)
        with urllib.request.urlopen(url) as response:
            data = response.read()
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            archive.extractall(target)
    return target


def find_debug_file(folder: Path, target: str, name: str) -> Path | None:
    for candidate in (folder / target / f'{name}.debug', folder / f'{name}.debug', folder):
        if candidate.is_file():
            return candidate
    return None


def addr2line_tool() -> str:
    for tool in ('addr2line', 'llvm-addr2line', 'llvm-symbolizer'):
        path = shutil.which(tool)
        if path:
            return path
    sys.exit('addr2line not found: install binutils or LLVM')


def resolve(debug_file: Path, addresses: list[int]) -> list[list[tuple[str, str]]]:
    output = subprocess.run(
        [addr2line_tool(), '-f', '-C', '-i', '-a', '-e', str(debug_file)] + [hex(address) for address in addresses],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.splitlines()
    results = []
    for line in output:
        if line.startswith('0x'):
            results.append([])
        elif results:
            if results[-1] and results[-1][-1][1] is None:
                results[-1][-1] = (results[-1][-1][0], line)
            else:
                results[-1].append((line, None))
    return results


def short_path(location: str) -> str:
    match = SOURCE_PATH.match(location)
    return match.group(1) if match else location


def function_name(signature: str) -> str:
    depth = 0
    for index, char in enumerate(signature):
        if char == '(' and depth == 0 and index > 0:
            return signature[:index].strip()
        if char == '<':
            depth += 1
        elif char == '>':
            depth -= 1
    return signature.strip()


def main():
    parser = argparse.ArgumentParser(description='Add file and line numbers to the NTgCalls frames of a gdb backtrace sent in an issue.')
    parser.add_argument('backtrace', help='gdb output with "bt" and "info sharedlibrary", for example gdb.txt')
    parser.add_argument('--version', help='NTgCalls version the reporter used, for example 4.0.0-beta01')
    parser.add_argument('--platform', help='platform of the symbols archive, for example linux-x86_64; detected for Python wheels')
    parser.add_argument('--symbols', help='use a local symbols archive, folder or .debug file instead of downloading it')
    args = parser.parse_args()

    lines, bases = parse_backtrace(Path(args.backtrace).read_text(errors='replace'))
    frames = []
    for index, line in enumerate(lines):
        match = FRAME.match(line.strip())
        if match and library_target(os.path.basename(match.group(4))):
            frames.append((index, int(match.group(2), 16), match.group(3), os.path.basename(match.group(4))))
    if not frames:
        sys.exit('No NTgCalls frames found in the backtrace')

    resolved = {}
    for name in sorted({frame[3] for frame in frames}):
        if name not in bases:
            sys.exit(f'"info sharedlibrary" output for {name} is missing: ask the reporter to run it after "bt"')
        platform = args.platform or library_platform(name)
        if not args.symbols and (not args.version or not platform):
            sys.exit('Pass --version, and --platform for the Node.js and C libraries')
        debug_file = find_debug_file(symbols_folder(args.version or 'local', platform or 'local', args.symbols), library_target(name), name)
        if not debug_file:
            sys.exit(f'No debug symbols for {name} in that archive')
        library_frames = [frame for frame in frames if frame[3] == name]
        results = resolve(debug_file, [address - bases[name] - 1 for _, address, _, _ in library_frames])
        for frame, result in zip(library_frames, results):
            resolved[frame[0]] = (frame[2], result)

    mismatches = 0
    for index, line in enumerate(lines):
        if not line.startswith('#'):
            continue
        print(line)
        if index not in resolved:
            continue
        expected, chain = resolved[index]
        for position, (function, location) in enumerate(chain):
            if not location or '?' in location:
                continue
            suffix = f'  (inlined {function_name(function)})' if position < len(chain) - 1 else ''
            print(f'        at {short_path(location)}{suffix}')
        if chain and function_name(chain[-1][0]) != function_name(expected):
            mismatches += 1
    sys.stdout.flush()
    if mismatches:
        print(f'\nWarning: {mismatches} frames resolved to a different function than gdb printed; the symbols are probably from another version or platform.', file=sys.stderr)


main()
