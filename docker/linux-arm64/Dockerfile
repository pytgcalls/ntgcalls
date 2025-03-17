FROM quay.io/pypa/manylinux2014_aarch64:latest
RUN yum install -y \
    git \
    make \
    alsa-lib-devel \
    pulseaudio-libs-devel \
    mesa-libgbm-devel \
    libXcomposite-devel \
    libXrandr-devel \
    libXtst-devel
ENV MAKEFLAGS="-j$(nproc)"
ENV CFLAGS="-march=native -O3"
ENV CXXFLAGS="-march=native -O3"
RUN git clone --depth=1 -b release/20.x https://github.com/llvm/llvm-project.git \
    && cd llvm-project \
    && mkdir build \
    && cd build \
    && cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release ../llvm \
    && make -j$(nproc) \
    && make install \
    && cd ../.. \
    && rm -rf llvm-project \
    && ln -s /usr/local/bin/clang++ /usr/local/bin/clang++-19 \
    && ln -s /usr/local/bin/python3.12 /usr/local/bin/python3
