import base64
import platform
import shutil
import multiprocessing
import os
import re
import subprocess
import sys
from pathlib import Path
from urllib.request import urlopen
from setuptools import Extension, setup, Command
from setuptools.command.build_ext import build_ext

base_path = os.path.abspath(os.path.dirname(__file__))

with open(os.path.join(base_path, 'CMakeLists.txt'), 'r', encoding='utf-8') as f:
    regex = re.compile(r'VERSION ([A-Za-z0-9.]+)', re.MULTILINE)
    version = re.findall(regex, f.read())[1]

    if version.count('.') == 3:
        major, minor, path_, tweak = version.split('.')
        version = f'{major}.{minor}.{path_}.dev{tweak}'


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = "") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


def install_clang(path: Path):
    url = 'https://chromium.googlesource.com/chromium/src/tools/+/refs/heads/main/clang/scripts/update.py?format=text'
    os.mkdir(path)
    download_py = Path(path, 'download.py')
    with urlopen(url) as response:
        with open(download_py, 'w') as file:
            file.write(base64.b64decode(response.read()).decode('utf-8'))

    subprocess.run(
        [sys.executable, download_py, '--output-dir', path],
        check=True
    )


def get_os():
    return subprocess.run(["uname", "-o"], stdout=subprocess.PIPE, text=True).stdout.strip()


def get_os_cmake_args():
    if sys.platform.startswith("win32"):
        pass
    elif sys.platform.startswith("darwin"):
        return [
            "-DCMAKE_OSX_ARCHITECTURES=arm64",
            "-G",
            "Xcode",
        ]
    elif get_os() == "Android":
        raise NotImplementedError("Android is not supported yet")
    elif sys.platform.startswith("linux"):
        clang_c, clang_cxx = "clang-17", "clang++-17"
        if platform.processor() != 'aarch64':
            clang_path = Path(Path.cwd(), 'clang')
            if not Path(clang_path, 'bin').exists():
                install_clang(clang_path)
            clang_c = Path(clang_path, 'bin', 'clang')
            clang_cxx = Path(clang_path, 'bin', 'clang++')
        return [
            f"-DCMAKE_C_COMPILER={clang_c}",
            f"-DCMAKE_CXX_COMPILER={clang_cxx}",
        ]
    return []


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()
        cfg = "RelWithDebInfo" if "dev" in version else "Release"
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={cfg}",
            f"-DPY_VERSION_INFO={version}",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}",
        ]
        build_args = [
            "--config", cfg,
            f"-j{multiprocessing.cpu_count()}",
        ]
        cmake_args += get_os_cmake_args()

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        subprocess.run(
            ["cmake", ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=build_temp, check=True
        )


class SharedCommand(Command):
    description = 'Generate shared-libs files'
    user_options = [
        ('no-preserve-cache', None, "Do not preserve cache"),
        ('debug', None, "Debug build"),
    ]

    # noinspection PyAttributeOutsideInit
    def initialize_options(self):
        self.no_preserve_cache = False
        self.debug = False

    def finalize_options(self):
        pass

    # noinspection PyMethodMayBeStatic
    def run(self):
        cfg = "RelWithDebInfo" if self.debug else "Release"
        cmake_args = [
            f'-DCMAKE_BUILD_TYPE={cfg}',
        ]
        cmake_args += get_os_cmake_args()
        build_args = [
            '--config', cfg,
            f'-j{multiprocessing.cpu_count()}',
        ]
        build_temp = Path('shared-build')
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        source_dir = os.path.dirname(os.path.abspath(__file__))
        subprocess.run(
            ['cmake', source_dir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=build_temp, check=True
        )
        release_path = Path(build_temp, 'ntgcalls')
        tmp_release_path = Path(release_path, cfg)

        build_output = Path("shared-output-debug" if self.debug else "shared-output")
        if build_output.exists():
            shutil.rmtree(build_output)
        build_output.mkdir(parents=True)
        include_output = Path(build_output, 'include')
        include_output.mkdir(parents=True)
        if tmp_release_path.exists():
            release_path = tmp_release_path
        for file in os.listdir(release_path):
            if file.endswith('.dll') or file.endswith('.so') or file.endswith('.dylib'):
                lib_output = Path(build_output, file)
                shutil.move(Path(release_path, file), lib_output)
                shutil.copy(Path(source_dir, 'include', 'ntgcalls.h'), include_output)

                if self.no_preserve_cache:
                    shutil.rmtree(build_temp)
                    boost_dir = Path(source_dir, 'deps', 'boost')
                    for boost_build in os.listdir(boost_dir):
                        if boost_build.startswith('boost_'):
                            shutil.rmtree(Path(boost_dir, boost_build))
                    print("Cleanup successfully")
                return
        raise FileNotFoundError("No library files found")


with open(os.path.join(base_path, 'README.md'), encoding='utf-8') as f:
    readme = f.read()

setup(
    name='ntgcalls',
    version=version,
    long_description=readme,
    long_description_content_type='text/markdown',
    url='https://github.com/pytgcalls/ntgcalls',
    author='Laky-64',
    author_email='iraci.matteo@gmail.com',
    classifiers=[
        'License :: OSI Approved :: '
        'GNU Lesser General Public License v3 (LGPLv3)',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: Python :: Implementation :: CPython',
    ],
    ext_modules=[CMakeExtension("ntgcalls")],
    cmdclass={
        'build_ext': CMakeBuild,
        'build_shared': SharedCommand
    },
    zip_safe=False,
    extras_require={"test": ["pytest>=6.0"]},
    python_requires=">=3.7",
)
