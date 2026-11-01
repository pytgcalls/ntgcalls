#!/bin/sh
# Experimental size compatibility for glibc-built dependencies on musl ARM64.
# Matching sizes does not establish full glibc/musl ABI compatibility.
set -eu

if [ "$(uname -m)" != aarch64 ]; then
    echo "glibc_pthread_abi: not aarch64, skipping"
    exit 0
fi

H=/usr/include/bits/alltypes.h

sed -i \
    -e '/} pthread_attr_t;/{s/?14:9\]/?16:9]/g;s/?7:9\]/?8:9]/}' \
    -e '/} pthread_mutex_t;/{s/?10:6\]/?12:6]/g;s/?5:6\]/?6:6]/}' \
    -e '/pthread_mutexattr_t;/s/{ unsigned __attr; }/{ unsigned __attr; unsigned __glibc_pad; }/' \
    -e '/pthread_condattr_t;/s/{ unsigned __attr; }/{ unsigned __attr; unsigned __glibc_pad; }/' \
    -e '/pthread_barrierattr_t;/s/{ unsigned __attr; }/{ unsigned __attr; unsigned __glibc_pad; }/' \
    "$H"

cat > /tmp/glibc_pthread_abi_check.c <<'EOF'
#include <pthread.h>
#include <semaphore.h>
_Static_assert(sizeof(pthread_attr_t) == 64, "pthread_attr_t");
_Static_assert(sizeof(pthread_mutex_t) == 48, "pthread_mutex_t");
_Static_assert(sizeof(pthread_mutexattr_t) == 8, "pthread_mutexattr_t");
_Static_assert(sizeof(pthread_cond_t) == 48, "pthread_cond_t");
_Static_assert(sizeof(pthread_condattr_t) == 8, "pthread_condattr_t");
_Static_assert(sizeof(pthread_rwlock_t) == 56, "pthread_rwlock_t");
_Static_assert(sizeof(pthread_rwlockattr_t) == 8, "pthread_rwlockattr_t");
_Static_assert(sizeof(pthread_barrier_t) == 32, "pthread_barrier_t");
_Static_assert(sizeof(pthread_barrierattr_t) == 8, "pthread_barrierattr_t");
_Static_assert(sizeof(sem_t) == 32, "sem_t");
int main(void) { return 0; }
EOF

cc -fsyntax-only /tmp/glibc_pthread_abi_check.c
rm /tmp/glibc_pthread_abi_check.c
echo "glibc_pthread_abi: pthread sizes match glibc aarch64"
