#!/bin/bash

set -e

set -v

export PATH=$DEPOT_TOOLS:$PATH

cd ${SOURCE_DIR}

if [ "$(uname)" == "Linux" ]; then
if [ "$TARGET_ARCH" == "arm" ]; then
  python3 build/linux/sysroot_scripts/install-sysroot.py --arch=arm
elif [ "$TARGET_ARCH" == "arm64" ]; then
  python3 build/linux/sysroot_scripts/install-sysroot.py --arch=arm64
else
  python3 build/linux/sysroot_scripts/install-sysroot.py --arch=amd64
fi
fi

# NOTE(mroberts): Running hooks generates this file, but running hooks also
# takes too long in CI; so do this manually.
(cd build/util && python3 lastchange.py -o LASTCHANGE)

gn gen ${BINARY_DIR} "--args=${GN_GEN_ARGS}"
