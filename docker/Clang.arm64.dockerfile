FROM quay.io/pypa/manylinux_2_28_aarch64:latest
COPY . /app
WORKDIR /app
RUN bash build.sh
WORKDIR /
RUN rm -rf /app