import configparser
import json
import multiprocessing
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from urllib.request import urlopen

from setuptools import Extension, setup, Command
from setuptools.command.build_ext import build_ext

base_path = os.path.abspath(os.path.dirname(__file__))
CMAKE_VERSION = '4.1.2'
TOOLS_PATH = Path(Path.cwd(), 'build_tools')


def cmake_path():
    return Path(TOOLS_PATH, f'cmake_{CMAKE_VERSION.replace(".", "_")}')


def cmake_bin():
    if sys.platform.startswith('linux'):
        return Path(cmake_path(), 'bin', 'cmake')
    return 'cmake'


def install_cmake(cmake_version: str):
    fixed_name = cmake_path()
    if Path(fixed_name, 'bin').exists():
        return
    if not fixed_name.exists():
        fixed_name.mkdir(parents=True)
    os_base = 'x86_64' if platform.machine() != 'aarch64' else 'aarch64'
    url = (f'https://github.com/Kitware/CMake/releases/download/v{cmake_version}/'
           f'cmake-{cmake_version}-linux-{os_base}.sh')
    download_sh = Path(fixed_name, 'install_cmake.sh')
    with urlopen(url) as response:
        with open(download_sh, 'wb') as file:
            file.write(response.read())

    subprocess.run(
        ['bash', download_sh, '--skip-license', f'--prefix={fixed_name}'],
        check=True,
    )


LIGHT_COMMANDS = {'matrix', 'list_targets'}
version = '0'
if not any(cmd in sys.argv for cmd in LIGHT_COMMANDS):
    with open(os.path.join(base_path, 'CMakeLists.txt'), 'r', encoding='utf-8') as f:
        if sys.platform.startswith('linux'):
            install_cmake(CMAKE_VERSION)
        regex = re.compile(r'VERSION ([0-9.]+)', re.MULTILINE)
        cmake_command = [
            str(cmake_bin()),
            f'-DPROJECT_VERSION={re.findall(regex, f.read())[1]}',
            '-P',
            'cmake/VersionUtil.cmake',
        ]
        if os.environ.get('PYPI_TAGS'):
            cmake_command.insert(-2, '-DPYPI_TAGS=ON')
        version = subprocess.run(cmake_command, capture_output=True, text=True).stderr.strip()
        version = re.sub(r'\x1b\[[0-9;]*m', '', version)

class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = '') -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


def release_kind():
    return 'RelWithDebInfo' if sys.platform.startswith('linux') else 'Release'


def host_arch():
    machine = platform.machine().lower()
    if machine in ('x86_64', 'amd64'):
        return 'x64'
    if machine in ('aarch64', 'arm64'):
        return 'arm64'
    return machine


def base_subst():
    return {
        'root': base_path,
        'cmake': str(cmake_bin()),
        'toolchain': str(Path(base_path, 'cmake', 'Toolchain.cmake')),
        'python': sys.executable,
        'config': release_kind(),
        'config_upper': release_kind().upper(),
        'jobs': str(multiprocessing.cpu_count()),
        'version': version,
        'platform': sys.platform,
    }


def parse_defines(raw):
    result = {}
    for item in (raw or '').split(';'):
        item = item.strip()
        if not item:
            continue
        key, _, value = item.partition('=')
        result[key.strip()] = value.strip()
    return result


def execute_cfg(target, subst, archs=('auto',), section='build', keys=None):
    parser = configparser.ConfigParser(interpolation=None)
    parser.read(TARGETS_DIR / target / 'build.cfg')
    if not parser.has_section(section):
        return
    workdir_tmpl = parser.get('options', 'workdir', fallback='{builddir}')
    base_env = os.environ.copy()
    base_env['CMAKE_BUILD_PARALLEL_LEVEL'] = subst['jobs']
    blocks = [parser[section][k] for k in (keys or parser[section])]
    for arch in archs:
        ctx = dict(subst)
        ctx['arch'] = arch
        ctx['target'] = str(TARGETS_DIR / target)
        env = dict(base_env)
        if parser.has_section('env'):
            for key, value in parser['env'].items():
                env[key.upper()] = value.format(**ctx)
        workdir = Path(workdir_tmpl.format(**ctx))
        workdir.mkdir(parents=True, exist_ok=True)
        for block in blocks:
            for line in block.strip().splitlines():
                line = line.strip()
                if not line:
                    continue
                subprocess.run(
                    shlex.split(line.format(**ctx)), cwd=workdir, check=True, env=env
                )


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        extdir = (Path.cwd() / self.get_ext_fullpath(ext.name)).parent.resolve()
        subst = base_subst()
        subst.update({
            'out': str(extdir),
            'builddir': str(Path(self.build_temp) / ext.name),
            'static': 'OFF',
            'binding': 'python',
        })
        execute_cfg('python', subst)


TARGETS_DIR = Path(base_path, 'targets')
ARCH_OVERRIDES = {
    'android': ['arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64'],
}
DEFAULT_TARGET = 'c'


def discover_targets():
    result = {}
    if TARGETS_DIR.is_dir():
        for entry in sorted(TARGETS_DIR.iterdir()):
            if entry.is_dir():
                result[entry.name] = ARCH_OVERRIDES.get(entry.name, ['auto'])
    return result


BINDING_TARGETS = discover_targets()


def target_options(target):
    parser = configparser.ConfigParser(interpolation=None)
    parser.read(TARGETS_DIR / target / 'build.cfg')
    return dict(parser['options']) if parser.has_section('options') else {}


def load_platforms():
    parser = configparser.ConfigParser(interpolation=None)
    parser.read(TARGETS_DIR / 'platforms.cfg')
    return parser


def select_targets(spec):
    spec = (spec or '').strip()
    if spec.lower() == 'all':
        chosen = list(BINDING_TARGETS)
    else:
        chosen = [t.strip() for t in spec.split(',') if t.strip()]
    return [t for t in chosen if 'platforms' in target_options(t)]


class SharedCommand(Command):
    description = 'Generate shared-libs files'
    user_options = [
        ('no-preserve-cache', None, 'Do not preserve cache'),
        ('static', None, 'Static build'),
        ('target=', None, f'Binding target ({", ".join(BINDING_TARGETS)})'),
        ('defines=', None, 'Extra substitutions (key=value;key2=value2)'),
    ]

    # noinspection PyAttributeOutsideInit
    def initialize_options(self):
        self.no_preserve_cache = False
        self.static = False
        self.target = None
        self.defines = None

    # noinspection PyAttributeOutsideInit
    def finalize_options(self):
        if self.target is None:
            self.target = DEFAULT_TARGET
        if self.target not in BINDING_TARGETS:
            raise ValueError(
                f'Unknown target "{self.target}", available: {", ".join(BINDING_TARGETS)}'
            )

    def run(self):
        target = self.target
        build_lib = Path(base_path, 'build_lib')
        subst = base_subst()
        subst.update({
            'out': str(build_lib),
            'builddir': str(build_lib),
            'static': 'ON' if self.static else 'OFF',
            'binding': target,
        })
        subst.update(parse_defines(self.defines))
        execute_cfg(target, subst, BINDING_TARGETS[target])
        if self.no_preserve_cache:
            shutil.rmtree(build_lib, ignore_errors=True)
            print('Cleanup successfully')


class GenerateCommand(Command):
    description = 'Run the code generator to produce the schema without building'
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        subprocess.run(
            [cmake_bin(), f'-DROOT_DIR={base_path}', '-P',
             str(Path(base_path, 'cmake', 'codegen', 'RunCodegen.cmake'))],
            check=True,
        )


class PublishCommand(Command):
    description = 'Publish a binding through its build.cfg [publish] phase'
    user_options = [
        ('target=', None, f'Binding target ({", ".join(BINDING_TARGETS)})'),
        ('phase=', None, 'Publish phase to run (artifact, final)'),
        ('platform=', None, 'Target platform (defaults to host)'),
        ('arch=', None, 'Target arch (defaults to host)'),
        ('set-version=', None, 'Override the published version'),
        ('defines=', None, 'Extra substitutions (key=value;key2=value2)'),
    ]

    # noinspection PyAttributeOutsideInit
    def initialize_options(self):
        self.target = None
        self.phase = 'final'
        self.platform = None
        self.arch = None
        self.set_version = None
        self.defines = None

    # noinspection PyAttributeOutsideInit
    def finalize_options(self):
        if self.target is None:
            self.target = DEFAULT_TARGET
        if self.target not in BINDING_TARGETS:
            raise ValueError(
                f'Unknown target "{self.target}", available: {", ".join(BINDING_TARGETS)}'
            )

    def run(self):
        build_lib = Path(base_path, 'build_lib')
        subst = base_subst()
        subst.update({
            'out': str(build_lib),
            'builddir': str(build_lib),
            'static': 'OFF',
            'binding': self.target,
        })
        if self.platform:
            subst['platform'] = self.platform
        if self.set_version:
            subst['version'] = self.set_version
        subst.update(parse_defines(self.defines))
        archs = (self.arch,) if self.arch else ('auto',)
        execute_cfg(self.target, subst, archs, section='publish', keys=[self.phase])


class ListTargetsCommand(Command):
    description = 'List the binding targets that declare a build matrix'
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        print(json.dumps(select_targets('all')))


class MatrixCommand(Command):
    description = 'Emit the CI build matrix as JSON'
    user_options = [
        ('targets=', None, 'Comma list of targets or "all"'),
        ('platforms=', None, 'Comma list of platform filters or "all"'),
        ('final', None, 'Emit the final-publish matrix instead of the build matrix'),
    ]

    # noinspection PyAttributeOutsideInit
    def initialize_options(self):
        self.targets = 'all'
        self.platforms = 'all'
        self.final = False

    def finalize_options(self):
        pass

    def run(self):
        plat_filter = None
        if self.platforms and self.platforms.strip().lower() != 'all':
            plat_filter = {p.strip() for p in self.platforms.split(',') if p.strip()}
        registry = load_platforms()
        include = []
        for target in select_targets(self.targets):
            opt = target_options(target)
            publish = opt.get('publish', 'none')
            if self.final:
                if publish in ('final', 'both'):
                    include.append({'target': target, 'tools': opt.get('tools', '')})
                continue
            for plat in [p.strip() for p in opt.get('platforms', '').split(',') if p.strip()]:
                if plat_filter and plat not in plat_filter:
                    continue
                if not registry.has_section(plat):
                    continue
                row = {
                    'target': target,
                    'platform': plat,
                    'publish': publish,
                    'tools': opt.get('tools', ''),
                    'output': opt.get('output', ''),
                    'output2': opt.get('output2', ''),
                }
                row.update(registry[plat])
                shared_name = row.get('shared_name', plat)
                row['artifact'] = opt.get('artifact', '').format(
                    shared_name=shared_name, target=target, platform=plat
                )
                row['artifact2'] = opt.get('artifact2', '').format(
                    shared_name=shared_name, target=target, platform=plat
                )
                include.append(row)
        print(json.dumps({'include': include}))


setup(
    version=version,
    ext_modules=[CMakeExtension('ntgcalls')],
    cmdclass={
        'build_ext': CMakeBuild,
        'build_lib': SharedCommand,
        'publish_lib': PublishCommand,
        'list_targets': ListTargetsCommand,
        'matrix': MatrixCommand,
        'generate': GenerateCommand
    },
    zip_safe=False,
)
