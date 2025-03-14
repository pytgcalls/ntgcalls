import multiprocessing
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path
from urllib.request import urlopen

from setuptools import Extension, setup, Command
from setuptools.command.build_ext import build_ext

base_path = os.path.abspath(os.path.dirname(__file__))
CMAKE_VERSION = '3.29.6'
TOOLS_PATH = Path(Path.cwd(), 'build_tools')


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


def cmake_path():
    return Path(TOOLS_PATH, f'cmake_{CMAKE_VERSION.replace(".", "_")}')


def cmake_bin():
    if sys.platform.startswith('linux'):
        return Path(cmake_path(), 'bin', 'cmake')
    return 'cmake'


def release_kind():
    return 'RelWithDebInfo' if sys.platform.startswith('linux') else 'Release'


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


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        if sys.platform.startswith('linux'):
            install_cmake(CMAKE_VERSION)
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()
        cfg = release_kind()
        cmake_args = [
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}',
            f'-DPYTHON_EXECUTABLE={sys.executable}',
            f'-DCMAKE_BUILD_TYPE={cfg}',
            f'-DPY_VERSION_INFO={version}',
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}',
            f'-DCMAKE_TOOLCHAIN_FILE={Path(Path.cwd(), "cmake", "Toolchain.cmake")}',
        ]

        build_args = [
            '--config', cfg,
            f'-j{multiprocessing.cpu_count()}',
        ]

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        subprocess.run(
            [cmake_bin(), ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            [cmake_bin(), '--build', '.', '--target', 'clean_objects'], cwd=build_temp, check=True
        )
        subprocess.run(
            [cmake_bin(), '--build', '.', *build_args], cwd=build_temp, check=True
        )


class SharedCommand(Command):
    description = 'Generate shared-libs files'
    user_options = [
        ('no-preserve-cache', None, 'Do not preserve cache'),
        ('static', None, 'Static build'),
        ('android', None, 'Android build'),
    ]

    # noinspection PyAttributeOutsideInit
    def initialize_options(self):
        self.no_preserve_cache = False
        self.static = False
        self.android = False

    def finalize_options(self):
        pass

    # noinspection PyMethodMayBeStatic
    def run(self):
        if sys.platform.startswith('linux'):
            install_cmake(CMAKE_VERSION)
        arch_outputs = [
            'auto',
        ]
        cmake_args = [
            f'-DCMAKE_BUILD_TYPE={release_kind()}',
            f'-DSTATIC_BUILD={"ON" if self.static else "OFF"}',
            f'-DCMAKE_TOOLCHAIN_FILE={Path(Path.cwd(), "cmake", "Toolchain.cmake")}',
        ]
        build_args = [
            '--config', release_kind(),
            f'-j{multiprocessing.cpu_count()}',
        ]
        build_temp = Path('build_lib')
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        source_dir = os.path.dirname(os.path.abspath(__file__))
        if self.android:
            arch_outputs = [
                'arm64-v8a',
                'armeabi-v7a',
                'x86',
                'x86_64',
            ]
        for arch in arch_outputs:
            new_cmake_args = cmake_args.copy()
            if arch != 'auto':
                new_cmake_args += [
                    f'-DANDROID_ABI={arch}',
                ]
            else:
                new_cmake_args += [
                    f'-DANDROID_ABI=OFF',
                ]

            subprocess.run(
                [cmake_bin(), source_dir, *new_cmake_args], cwd=build_temp, check=True
            )
            subprocess.run(
                [cmake_bin(), '--build', '.', '--target', 'clean_objects'], cwd=build_temp, check=True
            )
            subprocess.run(
                [cmake_bin(), '--build', '.', *build_args], cwd=build_temp, check=True
            )
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
