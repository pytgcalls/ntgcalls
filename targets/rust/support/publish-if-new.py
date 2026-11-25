import argparse
import subprocess
import sys
import urllib.error
import urllib.request


def in_crates_io(crate: str, version: str) -> bool:
    request = urllib.request.Request(
        f'https://crates.io/api/v1/crates/{crate}/{version}',
        headers={'User-Agent': 'ntgcalls-publish'},
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.status == 200
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return False
        raise


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--crate', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command:
        parser.error('missing the publish command')
    if in_crates_io(args.crate, args.version):
        print(f'skip {args.crate}@{args.version} (already on crates.io)', flush=True)
        return 0
    return subprocess.run(command).returncode


if __name__ == '__main__':
    sys.exit(main())
