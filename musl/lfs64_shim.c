/* musl >= 1.2.4 dropped the *64 functions and has no glibc internal names. */
#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <locale.h>
#include <resolv.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <wchar.h>
#include <pthread.h>

/* older musl headers turn these names into macros: remove them */
#undef open64
#undef openat64
#undef creat64
#undef fcntl64
#undef fopen64
#undef freopen64
#undef fseeko64
#undef ftello64
#undef ftruncate64
#undef truncate64
#undef lseek64
#undef pread64
#undef mmap64
#undef mkstemp64
#undef mkostemp64
#undef readdir64
#undef scandir64
#undef alphasort64
#undef nftw64
#undef statfs64
#undef fstatfs64
#undef statvfs64
#undef getrlimit64
#undef setrlimit64
#undef posix_fadvise64
#undef fallocate64
#undef sendfile64

#define W __attribute__((weak, visibility("default")))

W int open64(const char* p, int f, ...) {
    mode_t m = 0;
    if (f & (O_CREAT | O_TMPFILE)) {
        va_list a;
        va_start(a, f);
        m = va_arg(a, int);
        va_end(a);
    }
    return open(p, f, m);
}
W int openat64(int d, const char* p, int f, ...) {
    mode_t m = 0;
    if (f & (O_CREAT | O_TMPFILE)) {
        va_list a;
        va_start(a, f);
        m = va_arg(a, int);
        va_end(a);
    }
    return openat(d, p, f, m);
}
W int creat64(const char* p, mode_t m) {
    return creat(p, m);
}
W int fcntl64(int fd, int cmd, ...) {
    va_list a;
    va_start(a, cmd);
    void* arg = va_arg(a, void*);
    va_end(a);
    return fcntl(fd, cmd, arg);
}
W FILE* fopen64(const char* p, const char* m) {
    return fopen(p, m);
}
W FILE* freopen64(const char* p, const char* m, FILE* f) {
    return freopen(p, m, f);
}
W int fseeko64(FILE* f, off_t o, int w) {
    return fseeko(f, o, w);
}
W off_t ftello64(FILE* f) {
    return ftello(f);
}
W int ftruncate64(int fd, off_t l) {
    return ftruncate(fd, l);
}
W int truncate64(const char* p, off_t l) {
    return truncate(p, l);
}
W off_t lseek64(int fd, off_t o, int w) {
    return lseek(fd, o, w);
}
W ssize_t pread64(int fd, void* b, size_t n, off_t o) {
    return pread(fd, b, n, o);
}
W void* mmap64(void* a, size_t l, int p, int f, int fd, off_t o) {
    return mmap(a, l, p, f, fd, o);
}
W int mkstemp64(char* t) {
    return mkstemp(t);
}
W int mkostemp64(char* t, int f) {
    return mkostemp(t, f);
}
W struct dirent* readdir64(DIR* d) {
    return readdir(d);
}
W int scandir64(const char* d, struct dirent*** n, int (*s)(const struct dirent*), int (*c)(const struct dirent**, const struct dirent**)) {
    return scandir(d, n, s, c);
}
W int alphasort64(const struct dirent** a, const struct dirent** b) {
    return alphasort(a, b);
}
W int nftw64(const char* p, int (*fn)(const char*, const struct stat*, int, struct FTW*), int n, int f) {
    return nftw(p, fn, n, f);
}
W int statfs64(const char* p, struct statfs* b) {
    return statfs(p, b);
}
W int fstatfs64(int fd, struct statfs* b) {
    return fstatfs(fd, b);
}
W int statvfs64(const char* p, struct statvfs* b) {
    return statvfs(p, b);
}
W int getrlimit64(int r, struct rlimit* l) {
    return getrlimit(r, l);
}
W int setrlimit64(int r, const struct rlimit* l) {
    return setrlimit(r, l);
}
W int posix_fadvise64(int fd, off_t o, off_t l, int a) {
    return posix_fadvise(fd, o, l, a);
}
W int fallocate64(int fd, int m, off_t o, off_t l) {
    return fallocate(fd, m, o, l);
}
W ssize_t sendfile64(int o, int i, off_t* off, size_t n) {
    return sendfile(o, i, off, n);
}

/* glibc internal names */
W int __xstat64(int v, const char* p, struct stat* b) {
    return stat(p, b);
}
W int __lxstat64(int v, const char* p, struct stat* b) {
    return lstat(p, b);
}
W int __fxstat64(int v, int fd, struct stat* b) {
    return fstat(fd, b);
}
W struct cmsghdr* __cmsg_nxthdr(struct msghdr* m, struct cmsghdr* c) {
    return CMSG_NXTHDR(m, c);
}
W int __dn_expand(const unsigned char* m, const unsigned char* e, const unsigned char* s, char* d, int l) {
    return dn_expand(m, e, s, d, l);
}
W void __longjmp_chk(jmp_buf env, int v) {
    longjmp(env, v);
}
W size_t __mbrlen(const char* s, size_t n, mbstate_t* ps) {
    return mbrlen(s, n, ps);
}
W long __sysconf(int n) {
    return sysconf(n);
}
W long long strtoll_l(const char* s, char** e, int b, locale_t l) {
    return strtoll(s, e, b);
}
W unsigned long long strtoull_l(const char* s, char** e, int b, locale_t l) {
    return strtoull(s, e, b);
}

/* Chromium libc++ std::__Cr::mutex::~mutex() */
W void _ZNSt4__Cr5mutexD1Ev(pthread_mutex_t* m) {
    pthread_mutex_destroy(m);
}
