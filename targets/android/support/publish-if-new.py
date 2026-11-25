import argparse
import base64
import json
import os
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request


def in_maven_central(group: str, artifact: str, version: str) -> bool:
    path = '/'.join([*group.split('.'), artifact, version])
    url = f'https://repo1.maven.org/maven2/{path}/{artifact}-{version}.pom'
    request = urllib.request.Request(url, method='HEAD')
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.status == 200
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return False
        raise


def in_central_portal(group: str, artifact: str, version: str) -> bool:
    username = os.environ.get('ORG_GRADLE_PROJECT_mavenCentralUsername')
    password = os.environ.get('ORG_GRADLE_PROJECT_mavenCentralPassword')
    if not username or not password:
        return False
    query = urllib.parse.urlencode(
        {'namespace': group, 'name': artifact, 'version': version},
    )
    token = base64.b64encode(f'{username}:{password}'.encode()).decode()
    request = urllib.request.Request(
        f'https://central.sonatype.com/api/v1/publisher/published?{query}',
        headers={'Authorization': f'Bearer {token}'},
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return bool(json.load(response).get('published'))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--group', required=True)
    parser.add_argument('--artifact', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command:
        parser.error('missing the publish command')
    spec = f'{args.group}:{args.artifact}:{args.version}'
    if in_maven_central(args.group, args.artifact, args.version):
        print(f'skip {spec} (already in Maven Central)', flush=True)
        return 0
    if in_central_portal(args.group, args.artifact, args.version):
        print(f'skip {spec} (already published on the Central Portal)', flush=True)
        return 0
    return subprocess.run(command).returncode


if __name__ == '__main__':
    sys.exit(main())
