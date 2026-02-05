FROM quay.io/pypa/musllinux_1_2_x86_64:latest
COPY . /app
WORKDIR /app
RUN bash build.sh
WORKDIR /
RUN rm -rf /app