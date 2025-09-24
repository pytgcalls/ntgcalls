source /dev/stdin <<< "$(curl -s https://raw.githubusercontent.com/pytgcalls/build-toolkit/refs/heads/master/build-toolkit.sh)"

ulimit -n 1048576

import libraries.properties
import patch-nasm.sh

if command -v yum >/dev/null 2>&1; then
  yum install -y \
      git \
      make \
      wget \
      alsa-lib-devel \
      pulseaudio-libs-devel \
      flex \
      elfutils-libelf-devel \
      texinfo
else
  apk add --no-cache \
      bash \
      build-base \
      git \
      make \
      wget \
      alsa-lib-dev \
      pulseaudio-dev \
      flex \
      elfutils-dev \
      texinfo \
      findutils
fi

build_and_install "nasm" configure --setup-commands="patch_nasm"
build_and_install "llvm-project/llvm" cmake \
    -DLLVM_ENABLE_PROJECTS="libclc;clang" \
    -DLLVM_TARGETS_TO_BUILD="all" \
    -DLLVM_ENABLE_PIC=ON \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DCMAKE_BUILD_TYPE=Release

ln -s /usr/local/bin/python3.13 /usr/local/bin/python3 >&/dev/null