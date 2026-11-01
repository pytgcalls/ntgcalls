/* glibc compatibility functions for musl (prebuilt WebRTC/FFmpeg use them) */
#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>

/* ---- _FORTIFY_SOURCE *_chk functions ---- */
void* __memcpy_chk(void* d, const void* s, size_t n, size_t dl) {
    return memcpy(d, s, n);
}
void* __memmove_chk(void* d, const void* s, size_t n, size_t dl) {
    return memmove(d, s, n);
}
void* __memset_chk(void* d, int c, size_t n, size_t dl) {
    return memset(d, c, n);
}
char* __strcpy_chk(char* d, const char* s, size_t dl) {
    return strcpy(d, s);
}
char* __strncpy_chk(char* d, const char* s, size_t n, size_t dl) {
    return strncpy(d, s, n);
}
char* __strcat_chk(char* d, const char* s, size_t dl) {
    return strcat(d, s);
}
char* __strncat_chk(char* d, const char* s, size_t n, size_t dl) {
    return strncat(d, s, n);
}
char* __stpcpy_chk(char* d, const char* s, size_t dl) {
    return stpcpy(d, s);
}

int __vsnprintf_chk(char* s, size_t n, int f, size_t sl, const char* fmt, va_list ap) {
    return vsnprintf(s, n, fmt, ap);
}
int __snprintf_chk(char* s, size_t n, int f, size_t sl, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(s, n, fmt, ap);
    va_end(ap);
    return r;
}
int __vsprintf_chk(char* s, int f, size_t sl, const char* fmt, va_list ap) {
    return vsprintf(s, fmt, ap);
}
int __sprintf_chk(char* s, int f, size_t sl, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsprintf(s, fmt, ap);
    va_end(ap);
    return r;
}
int __vfprintf_chk(FILE* fp, int f, const char* fmt, va_list ap) {
    return vfprintf(fp, fmt, ap);
}
int __fprintf_chk(FILE* fp, int f, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(fp, fmt, ap);
    va_end(ap);
    return r;
}
int __vprintf_chk(int f, const char* fmt, va_list ap) {
    return vprintf(fmt, ap);
}
int __printf_chk(int f, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

long __fdelt_chk(long d) {
    return d / (8 * (long) sizeof(long));
}
ssize_t __read_chk(int fd, void* b, size_t n, size_t bl) {
    return read(fd, b, n);
}
int __poll_chk(struct pollfd* f, nfds_t n, int t, size_t fl) {
    return poll(f, n, t);
}
void __explicit_bzero_chk(void* p, size_t n, size_t pl) {
    explicit_bzero(p, n);
}

/* ---- resolver ---- */
int __res_nquery(void* st, const char* name, int cls, int type, unsigned char* ans, int len) {
    return res_query(name, cls, type, ans, len);
}
int __res_nsearch(void* st, const char* name, int cls, int type, unsigned char* ans, int len) {
    return res_search(name, cls, type, ans, len);
}
int __res_ninit(void* st) {
    return res_init();
}
void __res_nclose(void* st) {}

/* ---- misc glibc ---- */
char __libc_single_threaded = 0;
const char* gnu_get_libc_version(void) {
    return "2.31";
}
int __register_atfork(void (*p)(void), void (*pa)(void), void (*c)(void), void* dso) {
    return pthread_atfork(p, pa, c);
}

/* ---- glibc 2.38 C23 aliases ---- */
long __isoc23_strtol(const char* s, char** e, int b) {
    return strtol(s, e, b);
}
unsigned long __isoc23_strtoul(const char* s, char** e, int b) {
    return strtoul(s, e, b);
}
long long __isoc23_strtoll(const char* s, char** e, int b) {
    return strtoll(s, e, b);
}
unsigned long long __isoc23_strtoull(const char* s, char** e, int b) {
    return strtoull(s, e, b);
}
int __isoc23_sscanf(const char* s, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsscanf(s, fmt, ap);
    va_end(ap);
    return r;
}
