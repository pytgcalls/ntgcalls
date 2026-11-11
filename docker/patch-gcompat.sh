patch_gcompat() {
  sed -i "s:^LOADER_NAME = .*:LOADER_NAME = ld-linux-aarch64.so.1:" Makefile
  sed -i "1i WITH_LIBUCONTEXT = 1\nLINKER_PATH = /lib/ld-musl-aarch64.so.1" Makefile
}
