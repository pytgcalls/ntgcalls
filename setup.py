import multiprocessing
import os
import platform
import re
import subprocess
import sys
import base64
from pathlib import Path
from urllib.request import urlopen
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

base_path = os.path.abspath(os.path.dirname(__file__))


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

class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()
        debug = int(os.environ.get("DEBUG", 0)) if self.debug is None else self.debug
        cfg = "Debug" if debug else "Release"
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={cfg}",
            f"-DPY_VERSION_INFO={self.distribution.get_version()}",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}",
        ]
        build_args = [
            "--config", cfg,
            f"-j{multiprocessing.cpu_count()}",
        ]

        if get_os() == "Android":
            raise NotImplementedError("Android is not supported yet")
        elif sys.platform.startswith("darwin"):
            cmake_args += [
                "-DCMAKE_OSX_ARCHITECTURES=arm64",
                "-G",
                "Xcode",
            ]
        elif sys.platform.startswith("linux"):
            clang_c = 'clang'
            clang_cxx = 'clang++'
            if platform.processor() is not 'arm64':
                clang_path = Path(Path.cwd(), 'clang')
                if not Path(clang_path, 'bin').exists():
                    install_clang(clang_path)
                clang_c = Path(clang_path, 'bin', clang_c)
                clang_cxx = Path(clang_path, 'bin', clang_cxx)
            cmake_args += [
                f"-DCMAKE_C_COMPILER={clang_c}",
                f"-DCMAKE_CXX_COMPILER={clang_cxx}",
            ]
        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)
        subprocess.run(
            ["cmake", ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=build_temp, check=True
        )


with open(os.path.join(base_path, 'CMakeLists.txt'), 'r', encoding='utf-8') as f:
    regex = re.compile(r'VERSION ([A-Za-z0-9.]+)', re.MULTILINE)
    version = re.findall(regex, f.read())[1]

    if version.count('.') == 3:
        major, minor, path_, tweak = version.split('.')
        version = f'{major}.{minor}.{path_}.dev{tweak}'

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
        'Programming Language :: Python :: Implementation :: CPython',
        'Programming Language :: Python :: Implementation :: PyPy',
    ],
    ext_modules=[CMakeExtension("ntgcalls")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    extras_require={"test": ["pytest>=6.0"]},
    python_requires=">=3.7",
)
