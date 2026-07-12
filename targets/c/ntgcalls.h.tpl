@config c_prefix = ntg
@typemap Vector<bytes> = ntg_bytes
@typemap Vector<*> = *
@typemap long = int64_t
@typemap ulong = uint64_t
@typemap int = int32_t
@typemap uint = uint32_t
@typemap int8 = int8_t
@typemap uint8 = uint8_t
@typemap int16 = int16_t
@typemap uint16 = uint16_t
@typemap bool = bool
@typemap double = double
@typemap string = char*
@typemap bytes = uint8_t
@typemap Void = void
@typemap media.* = ntg_*
@typemap models.* = ntg_*
@typemap p2p.* = ntg_*
@typemap e2e.* = ntg_*
@typemap instances.* = ntg_*
@typemap * = ntg_*
@out @{config.self_dir}/ntgcalls.h
#pragma once

#if defined _WIN32 || defined __CYGWIN__
#ifdef NTG_EXPORTS
#define NTG_C_EXPORT __declspec(dllexport)
#else
#define NTG_C_EXPORT __declspec(dllimport)
#endif
#else
#ifdef NTG_EXPORTS
#define NTG_C_EXPORT __attribute__((visibility("default")))
#else
#define NTG_C_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct ntg_bytes {
    uint8_t* data;
    size_t len;
} ntg_bytes;

typedef enum {
    NTG_OK = 0,
    NTG_ERR_UNKNOWN = -1,
    NTG_ERR_NULL_POINTER = -2,
@for e in excs
    NTG_ERR_@{e.name|snake|upper} = @{e.code},
@end
} ntg_result;

typedef enum {
    NTG_LOG_DEBUG = 1,
    NTG_LOG_INFO = 2,
    NTG_LOG_WARNING = 4,
    NTG_LOG_ERROR = 8
} ntg_log_level;

typedef enum {
    NTG_LOG_SOURCE_WEBRTC = 1,
    NTG_LOG_SOURCE_SELF = 2
} ntg_log_source;

typedef struct ntg_log_message {
    ntg_log_level level;
    ntg_log_source source;
    const char* file;
    uint32_t line;
    const char* message;
} ntg_log_message;

typedef void (*ntg_log_cb)(ntg_log_message message, void* user_data);

NTG_C_EXPORT void ntg_set_log_callback(ntg_log_cb callback, void* user_data);

NTG_C_EXPORT const char* ntg_get_version(void);

@for e in enums
@if e.emit
typedef enum {
@for mem in e.members
    NTG_@{e.name|snake|upper}_@{mem.disp|snake|upper},
@end
} ntg_@{e.name|snake};

@end
@end
@for s in structs
typedef struct ntg_@{s.name|snake} {
@for f in s.fields
@if f.bytes
    uint8_t* @{f.name|snake};
    size_t @{f.name|snake}_len;
@else
@if f.string
    char* @{f.name|snake};
@else
@if f.vector
    @{f.type|type|snake}* @{f.name|snake};
    size_t @{f.name|snake}_len;
@else
@if f.optional
    @{f.type|type|snake}* @{f.name|snake};
@else
    @{f.type|type|snake} @{f.name|snake};
@end
@end
@end
@end
@end
} ntg_@{s.name|snake};

@end
@for me in mapentries
typedef struct @{me.valtl|type|snake}_entry {
    @{me.keytl|type|snake} key;
    @{me.valtl|type|snake} value;
} @{me.valtl|type|snake}_entry;

@end
typedef struct ntg_instance ntg_instance;

NTG_C_EXPORT ntg_instance* ntg_instance_create(void);
NTG_C_EXPORT void ntg_instance_destroy(ntg_instance* handle);

@for s in structs
NTG_C_EXPORT void ntg_@{s.name|snake}_free(ntg_@{s.name|snake}* value);
@end
@for me in mapentries
NTG_C_EXPORT void @{me.valtl|type|snake}_entry_free(@{me.valtl|type|snake}_entry* entries, size_t len);
@end
NTG_C_EXPORT void ntg_string_free(char* value);
NTG_C_EXPORT void ntg_bytes_free(void* value);

@for c in classes
@for m in c.methods
@if m.iscb
@else
NTG_C_EXPORT ntg_result ntg_@{m.name|snake}(
@if m.static
@if m.isvoid
@if m.params
@else
    void
@end
@end
@else
@if m.isvoid
@if m.params
    ntg_instance* handle,
@else
    ntg_instance* handle
@end
@else
    ntg_instance* handle,
@end
@end
@if m.isvoid
@for p in m.params
@if p.bytes
    const uint8_t* @{p.name|snake}, size_t @{p.name|snake}_len@{p.sep}
@else
@if p.string
    const char* @{p.name|snake}@{p.sep}
@else
@if p.vector
    const @{p.type|type|snake}* @{p.name|snake}, size_t @{p.name|snake}_len@{p.sep}
@else
    @{p.type|type|snake} @{p.name|snake}@{p.sep}
@end
@end
@end
@end
@else
@for p in m.params
@if p.bytes
    const uint8_t* @{p.name|snake}, size_t @{p.name|snake}_len,
@else
@if p.string
    const char* @{p.name|snake},
@else
@if p.vector
    const @{p.type|type|snake}* @{p.name|snake}, size_t @{p.name|snake}_len,
@else
    @{p.type|type|snake} @{p.name|snake},
@end
@end
@end
@end
@if m.retstring
    char** out
@else
@if m.retbytes
    uint8_t** out, size_t* out_len
@else
@if m.retvector
    @{m.ret|type|snake}** out, size_t* out_len
@else
@if m.retmap
    @{m.retmapval|type|snake}_entry** out, size_t* out_len
@else
    @{m.ret|type|snake}* out
@end
@end
@end
@end
@end
);
@end
@end
@end
@for cb in callbacks
typedef void (*ntg_@{cb.name|snake}_cb)(
    ntg_instance* handle,
@for a in cb.cbargs
@if a.bytes
    const uint8_t* @{a.name|snake}, size_t @{a.name|snake}_len,
@else
@if a.string
    const char* @{a.name|snake},
@else
@if a.vector
    const @{a.type|type|snake}* @{a.name|snake}, size_t @{a.name|snake}_len,
@else
    @{a.type|type|snake} @{a.name|snake},
@end
@end
@end
@end
    void* user_data
);
NTG_C_EXPORT ntg_result ntg_on_@{cb.name|snake}(ntg_instance* handle, ntg_@{cb.name|snake}_cb callback, void* user_data);

@end
#ifdef __cplusplus
}
#endif
