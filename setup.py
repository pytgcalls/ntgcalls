import base64
import platform
import shutil
import multiprocessing
import os
import re
import subprocess
import sys
from datetime import datetime
from urllib.parse import quote
from pathlib import Path
from typing import Dict
from urllib.request import urlopen
from setuptools import Extension, setup, Command
from setuptools.command.build_ext import build_ext

base_path = os.path.abspath(os.path.dirname(__file__))
CLANG_VERSION = '18'
CMAKE_VERSION = '3.28.1'
TOOLS_PATH = Path(Path.cwd(), 'build_tools')


class CLangInfo:
    def __init__(
        self,
        timestamp: int,
        revision: str,
        sub_revision: str
    ):
        self.time = timestamp
        self.revision = revision
        self.sub_revision = sub_revision

    @staticmethod
    def to_timestamp(time: str) -> int:
        return int(datetime.strptime(time, '%Y-%m-%dT%H:%M:%S.%fZ').timestamp())


with open(os.path.join(base_path, 'CMakeLists.txt'), 'r', encoding='utf-8') as f:
    regex = re.compile(r'VERSION ([A-Za-z0-9.]+)', re.MULTILINE)
    version = re.findall(regex, f.read())[1]

    if version.count('.') == 3:
        major, minor, patch, tweak = version.split('.')
        version = f'{major}.{minor}.{patch}b{tweak}'


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = '') -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


def get_versions() -> Dict[str, CLangInfo]:
    versions: Dict[str, CLangInfo] = {}
    url_base = 'https://commondatastorage.googleapis.com/chromium-browser-clang/?delimiter=/&prefix=Linux_x64/'
    url_tmp = url_base
    while True:
        with urlopen(url_tmp) as response:
            res = response.read().decode('utf-8').replace('><Key>', '>\n<Key>')
            match_1 = re.findall(
                r'<Key>Linux_x64/clang-(llvmorg-([0-9]+)-.*?)-([0-9]+)\.tgz</Key>.*?'
                r'<LastModified>(.*?)</LastModified>',
                res,
            )
            for data in match_1:
                curr_time = CLangInfo.to_timestamp(data[3])
                if data[1] not in versions or curr_time > versions[data[1]].time:
                    versions[data[1]] = CLangInfo(curr_time, data[0], data[2])
            match_2 = re.findall(f'<NextMarker>(.*?)</NextMarker>', res)
            if len(match_2) == 0:
                break
            url_tmp = f'{url_base}&marker={quote(match_2[0])}'
    return versions


def cmake_path():
    return Path(TOOLS_PATH, f'cmake_{CMAKE_VERSION.replace(".", "_")}')


def cmake_bin():
    if sys.platform.startswith('linux'):
        return Path(cmake_path(), 'bin', 'cmake')
    return 'cmake'


def clang_path():
    return Path(TOOLS_PATH, f'clang_{CLANG_VERSION.replace(".", "_")}')


def install_cmake(cmake_version: str):
    fixed_name = cmake_path()
    if Path(fixed_name, 'bin').exists():
        return
    if not fixed_name.exists():
        os.mkdir(fixed_name)
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


def install_clang(clang_version: str):
    fixed_name = clang_path()
    if Path(fixed_name, 'bin').exists():
        return
    url = 'https://chromium.googlesource.com/chromium/src/tools/+/refs/heads/main/clang/scripts/update.py?format=text'

    if not fixed_name.exists():
        os.mkdir(fixed_name)
    version_info = get_versions()[clang_version]
    download_py = Path(fixed_name, 'download.py')
    with urlopen(url) as response:
        with open(download_py, 'w') as file:
            f_content = base64.b64decode(response.read()).decode('utf-8')
            f_content = re.sub(
                r"CLANG_REVISION = '(.*?)'",
                f"CLANG_REVISION = '{version_info.revision}'",
                f_content
            )
            f_content = re.sub(
                r"CLANG_SUB_REVISION = [0-9]+",
                f"CLANG_SUB_REVISION = {version_info.sub_revision}",
                f_content
            )
            f_content = re.sub(
                r"RELEASE_VERSION = '[0-9]+'",
                f"RELEASE_VERSION = '{clang_version}'",
                f_content
            )
            file.write(f_content)

    subprocess.run(
        [sys.executable, download_py, '--output-dir', fixed_name],
        check=True,
    )


def get_os():
    return subprocess.run(['uname', '-o'], stdout=subprocess.PIPE, text=True).stdout.strip()


def get_os_cmake_args():
    if sys.platform.startswith('win32'):
        pass
    elif sys.platform.startswith('darwin'):
        return [
            '-DCMAKE_OSX_ARCHITECTURES=arm64',
            '-G',
            'Xcode',
        ]
    elif get_os() == 'Android':
        raise NotImplementedError('Android is not supported yet')
    elif sys.platform.startswith('linux'):
        clang_c, clang_cxx = f'clang-{CLANG_VERSION}', f'clang++-{CLANG_VERSION}'

        if not TOOLS_PATH.exists():
            os.mkdir(TOOLS_PATH)

        install_cmake(CMAKE_VERSION)
        if platform.machine() != 'aarch64':
            install_clang(CLANG_VERSION)
            clang_c = Path(clang_path(), 'bin', 'clang')
            clang_cxx = Path(clang_path(), 'bin', 'clang++')
        return [
            f'-DCMAKE_C_COMPILER={clang_c}',
            f'-DCMAKE_CXX_COMPILER={clang_cxx}',
            '-DCMAKE_CXX_FLAGS=-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE',
        ]
    return []


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()
        cfg = 'RelWithDebInfo' if 'b' in version else 'Release'

        cmake_args = [
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}',
            f'-DPYTHON_EXECUTABLE={sys.executable}',
            f'-DCMAKE_BUILD_TYPE={cfg}',
            f'-DPY_VERSION_INFO={version}',
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}',
        ]

        if sys.platform.startswith('linux'):
            cxx_flags = ['-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE']
            cmake_args.append(f"-DCMAKE_CXX_FLAGS={' '.join(cxx_flags)}")

        build_args = [
            '--config', cfg,
            f'-j{multiprocessing.cpu_count()}',
        ]
        cmake_args += get_os_cmake_args()

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        subprocess.run(
            [cmake_bin(), ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            [cmake_bin(), '--build', '.', *build_args], cwd=build_temp, check=True
        )


class SharedCommand(Command):
    description = 'Generate shared-libs files'
    user_options = [
        ('no-preserve-cache', None, 'Do not preserve cache'),
        ('debug', None, 'Debug build'),
        ('static', None, 'Static build'),
    ]

    # noinspection PyAttributeOutsideInit
    def initialize_options(self):
        self.no_preserve_cache = False
        self.debug = False
        self.static = False

    def finalize_options(self):
        pass

    # noinspection PyMethodMayBeStatic
    def run(self):
        cfg = 'RelWithDebInfo' if self.debug else 'Release'
        cmake_args = [
            f'-DCMAKE_BUILD_TYPE={cfg}',
            f'-DSTATIC_BUILD={"ON" if self.static else "OFF"}',
        ]
        cmake_args += get_os_cmake_args()
        build_args = [
            '--config', cfg,
            f'-j{multiprocessing.cpu_count()}',
        ]
        build_type = 'static' if self.static else 'shared'
        config_type = 'debug' if self.debug else 'release'
        build_temp = Path('build_lib')
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        source_dir = os.path.dirname(os.path.abspath(__file__))
        subprocess.run(
            [cmake_bin(), source_dir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            [cmake_bin(), '--build', '.', *build_args], cwd=build_temp, check=True
        )
        release_path = Path(build_temp, 'ntgcalls')
        tmp_release_path = Path(release_path, cfg)

        build_output = Path(
            f'{build_type}-output',
            config_type,
        )
        if build_output.exists():
            shutil.rmtree(build_output)
        build_output.mkdir(parents=True)
        include_output = Path(build_output, 'include')
        include_output.mkdir(parents=True)
        if tmp_release_path.exists():
            release_path = tmp_release_path
        for file in os.listdir(release_path):
            target_file = Path(build_output, file)
            if file.endswith(('.lib', '.dll', '.so', '.dylib', '.a')):
                shutil.move(Path(release_path, file), target_file)
        shutil.copy(Path(source_dir, 'include', 'ntgcalls.h'), include_output)
        if self.no_preserve_cache:
            shutil.rmtree(build_temp)
            boost_dir = Path(source_dir, 'deps', 'boost')
            for boost_build in os.listdir(boost_dir):
                if boost_build.startswith('boost_'):
                    shutil.rmtree(Path(boost_dir, boost_build))
            print('Cleanup successfully')


setup(
    version=version,
    ext_modules=[CMakeExtension('ntgcalls')],
    cmdclass={
        'build_ext': CMakeBuild,
        'build_lib': SharedCommand
    },
    zip_safe=False,
)
