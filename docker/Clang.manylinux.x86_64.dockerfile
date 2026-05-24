FROM quay.io/pypa/manylinux_2_28_x86_64:latest
COPY . /app
WORKDIR /app
RUN bash build.sh
WORKDIR /
RUN rm -rf /app