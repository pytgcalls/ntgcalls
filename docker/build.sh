source /dev/stdin <<< "$(curl -s https://raw.githubusercontent.com/pytgcalls/build-toolkit/refs/heads/master/build-toolkit.sh)"

ulimit -n 1048576

import libraries.properties
import patch-nasm.sh
import patch-gcompat.sh
import bison-setup.sh

if command -v yum >/dev/null 2>&1; then
  yum install -y \
      git \
      make \
      wget \
      alsa-lib-devel \
      pulseaudio-libs-devel \
      flex \
      elfutils-libelf-devel \
      texinfo \
      perl-Compress-Zlib \
      gettext \
      gettext-devel \
      gperf \
      help2man
else
  apk add --no-cache \
      bash \
      build-base \
      coreutils \
      git \
      make \
      wget \
      alsa-lib-dev \
      pulseaudio-dev \
      flex \
      elfutils-dev \
      texinfo \
      findutils \
      zlib-dev  \
      perl \
      gperf \
      help2man
fi

if is_musl && is_arm64; then
  apk add --no-cache libucontext-dev musl-obstack-dev
  build_and_install "gcompat" make --setup-commands="patch_gcompat"
  sed -i "s:^ID=alpine:ID=musllinux:" /etc/os-release
fi

build_and_install "nasm" configure --setup-commands="patch_nasm"
build_and_install "bison" make --update-submodules --setup-commands="bison_setup"

llvm_target_args=()
if is_musl; then
  llvm_target_args+=("-DLLVM_DEFAULT_TARGET_TRIPLE=$(gcc -dumpmachine)")
fi

build_and_install "llvm-project/llvm" cmake \
    "${llvm_target_args[@]}" \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_ENABLE_RUNTIMES="compiler-rt" \
    -DCOMPILER_RT_BUILD_BUILTINS=ON \
    -DCOMPILER_RT_BUILD_SANITIZERS=OFF \
    -DCOMPILER_RT_BUILD_XRAY=OFF \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
    -DCOMPILER_RT_BUILD_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_MEMPROF=OFF \
    -DCOMPILER_RT_BUILD_ORC=OFF \
    -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_GWP_ASAN=OFF \
    -DLLVM_TARGETS_TO_BUILD="all" \
    -DLLVM_ENABLE_PIC=ON \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DCMAKE_BUILD_TYPE=Release

build_and_install "llvm-project/libclc" cmake \
    -DLIBCLC_STANDALONE_BUILD=ON \
    -DCMAKE_CLC_COMPILER=/usr/bin/clang \
    -DLIBCLC_USE_SPIRV_BACKEND=ON \
    -DLLVM_DEFAULT_TARGET_TRIPLE="spirv64-unknown-unknown" \
    -DCMAKE_BUILD_TYPE=Release

ln -s /usr/local/bin/python3.13 /usr/local/bin/python3 >&/dev/null

if ! is_musl; then
  ln -s /usr/lib64/libstdc++.so.6 /usr/lib64/libstdc++.so
fi
