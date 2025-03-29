FROM quay.io/pypa/manylinux_2_28_aarch64:latest
RUN yum install -y \
    git \
    make \
    nasm \
    alsa-lib-devel \
    pulseaudio-libs-devel \
    flex \
    elfutils-libelf-devel \
    texinfo

ENV MAKEFLAGS="-j$(nproc)"
ENV CFLAGS="-march=native -O3"
ENV CXXFLAGS="-march=native -O3"
RUN git clone --depth=1 -b release/20.x https://github.com/llvm/llvm-project.git \
    && cd llvm-project \
    && mkdir build \
    && cd build \
    && cmake -G "Unix Makefiles" \
        -DLLVM_ENABLE_PROJECTS="libclc;clang" \
        -DLLVM_TARGETS_TO_BUILD="all" \
        -DLLVM_ENABLE_PIC=ON \
        -DLLVM_BUILD_LLVM_DYLIB=ON \
        -DLLVM_LINK_LLVM_DYLIB=ON \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_INCLUDE_EXAMPLES=OFF \
        -DLLVM_ENABLE_RTTI=ON \
        -DLLVM_INCLUDE_BENCHMARKS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        ../llvm \
    && make -j$(nproc) \
    && make install \
    && cd ../.. \
    && rm -rf llvm-project \
    && ln -s /usr/local/bin/clang++ /usr/local/bin/clang++-20 \
    && ln -s /usr/local/bin/python3.12 /usr/local/bin/python3
