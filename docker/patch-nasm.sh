patch_nasm() {
  sed -i '/INSTALL_DATA.*nasm\.1/d' Makefile.in
  sed -i '/INSTALL_DATA.*ndisasm\.1/d' Makefile.in
}