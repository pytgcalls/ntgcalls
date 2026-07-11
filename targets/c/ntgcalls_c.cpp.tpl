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
@out @{config.self_dir}/ntgcalls_c.cpp
#include <cstdlib>
#include <cstring>
#include <map>

#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>

#include "ntgcalls.h"
#include "support/ntg_support.hpp"

#define HANDLE_EXCEPTIONS \
@for e in excs
@if e.parent
    catch (const @{e.cpp}&) { return NTG_ERR_@{e.name|snake|upper}; } \
@end
@end
@for e in excs
@if e.parent
@else
    catch (const @{e.cpp}&) { return NTG_ERR_@{e.name|snake|upper}; } \
@end
@end
    catch (...) { return NTG_ERR_UNKNOWN; }

struct ntg_instance {
    ntgcalls::NTgCalls* impl;
@for cb in callbacks
    ntg_@{cb.name|snake}_cb @{cb.method}_fn;
    void* @{cb.method}_ud;
@end
};

@for e in enums
@if e.emit
static ntg_@{e.name|snake} to_c_@{e.name}(@{e.cpp} value) {
    switch (value) {
@for mem in e.members
        case @{e.cpp}::@{mem.cpp}: return NTG_@{e.name|snake|upper}_@{mem.disp|snake|upper};
@end
    }
    return static_cast<ntg_@{e.name|snake}>(0);
}

static @{e.cpp} from_c_@{e.name}(ntg_@{e.name|snake} value) {
    switch (value) {
@for mem in e.members
        case NTG_@{e.name|snake|upper}_@{mem.disp|snake|upper}: return @{e.cpp}::@{mem.cpp};
@end
    }
    return static_cast<@{e.cpp}>(0);
}

@end
@end
@for s in structs
static ntg_@{s.name|snake} to_c_@{s.name}(const @{s.cpp}& value);
static @{s.cpp} from_c_@{s.name}(const ntg_@{s.name|snake}& value);
@end

@for s in structs
static ntg_@{s.name|snake} ntg_elem_to_c(const @{s.cpp}& value) { return to_c_@{s.name}(value); }
static @{s.cpp} ntg_elem_from_c(const ntg_@{s.name|snake}& value) { return from_c_@{s.name}(value); }
@end

static inline ntg_bytes ntg_elem_to_c(const bytes::binary& value) { ntg_bytes out; ntg_dup_bytes(value, &out.data, &out.len); return out; }
static inline bytes::binary ntg_elem_from_c(const ntg_bytes& value) { return ntg_to_cpp_bytes(value.data, value.len); }

template<typename CppT, typename CT>
static void ntg_to_c_vector(const std::vector<CppT>& in, CT** out, size_t* len) {
    *len = in.size();
    if (in.empty()) {
        *out = nullptr;
        return;
    }
    *out = static_cast<CT*>(malloc(sizeof(CT) * in.size()));
    for (size_t i = 0; i < in.size(); ++i) {
        (*out)[i] = ntg_elem_to_c(in[i]);
    }
}

template<typename CppT, typename CT>
static std::vector<CppT> ntg_from_c_vector(const CT* in, const size_t len) {
    std::vector<CppT> out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(ntg_elem_from_c(in[i]));
    }
    return out;
}

template<typename K, typename V, typename CEntry>
static void ntg_map_to_entries(const std::map<K, V>& in, CEntry** out, size_t* len) {
    *len = in.size();
    if (in.empty()) {
        *out = nullptr;
        return;
    }
    *out = static_cast<CEntry*>(malloc(sizeof(CEntry) * in.size()));
    size_t i = 0;
    for (const auto& [key, value] : in) {
        (*out)[i].key = ntg_elem_to_c(key);
        (*out)[i].value = ntg_elem_to_c(value);
        ++i;
    }
}

@for s in structs
static ntg_@{s.name|snake} to_c_@{s.name}(const @{s.cpp}& value) {
    ntg_@{s.name|snake} out;
@for f in s.fields
@if f.scalar
@if f.optional
    if (value.@{f.cpp}.has_value()) { out.@{f.name|snake} = static_cast<@{f.type|type|snake}*>(malloc(sizeof(@{f.type|type|snake}))); *out.@{f.name|snake} = *value.@{f.cpp}; } else { out.@{f.name|snake} = nullptr; }
@else
    out.@{f.name|snake} = value.@{f.cpp};
@end
@else
@if f.string
@if f.optional
    out.@{f.name|snake} = value.@{f.cpp}.has_value() ? ntg_dup_string(*value.@{f.cpp}) : nullptr;
@else
    out.@{f.name|snake} = ntg_dup_string(value.@{f.cpp});
@end
@else
@if f.bytes
@if f.optional
    if (value.@{f.cpp}.has_value()) { ntg_dup_bytes(*value.@{f.cpp}, &out.@{f.name|snake}, &out.@{f.name|snake}_len); } else { out.@{f.name|snake} = nullptr; out.@{f.name|snake}_len = 0; }
@else
    ntg_dup_bytes(value.@{f.cpp}, &out.@{f.name|snake}, &out.@{f.name|snake}_len);
@end
@else
@if f.vector
    ntg_to_c_vector(value.@{f.cpp}, &out.@{f.name|snake}, &out.@{f.name|snake}_len);
@else
@if f.optional
    if (value.@{f.cpp}.has_value()) { out.@{f.name|snake} = static_cast<@{f.type|type|snake}*>(malloc(sizeof(@{f.type|type|snake}))); *out.@{f.name|snake} = to_c_@{f.type|base}(*value.@{f.cpp}); } else { out.@{f.name|snake} = nullptr; }
@else
    out.@{f.name|snake} = to_c_@{f.type|base}(value.@{f.cpp});
@end
@end
@end
@end
@end
@end
    return out;
}

static @{s.cpp} from_c_@{s.name}(const ntg_@{s.name|snake}& value) {
    return @{s.cpp}(
@for f in s.fields
@if f.scalar
@if f.optional
        value.@{f.name|snake} ? std::optional<@{f.cpptype}>(*value.@{f.name|snake}) : std::nullopt@{f.sep}
@else
        value.@{f.name|snake}@{f.sep}
@end
@else
@if f.string
@if f.optional
        value.@{f.name|snake} ? std::optional<std::string>(value.@{f.name|snake}) : std::nullopt@{f.sep}
@else
        value.@{f.name|snake} ? std::string(value.@{f.name|snake}) : std::string()@{f.sep}
@end
@else
@if f.bytes
@if f.optional
        value.@{f.name|snake} ? std::optional<bytes::binary>(ntg_to_cpp_bytes(value.@{f.name|snake}, value.@{f.name|snake}_len)) : std::nullopt@{f.sep}
@else
        ntg_to_cpp_bytes(value.@{f.name|snake}, value.@{f.name|snake}_len)@{f.sep}
@end
@else
@if f.vector
        ntg_from_c_vector<@{f.elcpp}>(value.@{f.name|snake}, value.@{f.name|snake}_len)@{f.sep}
@else
@if f.optional
        value.@{f.name|snake} ? std::optional<@{f.cpptype}>(from_c_@{f.type|base}(*value.@{f.name|snake})) : std::nullopt@{f.sep}
@else
        from_c_@{f.type|base}(value.@{f.name|snake})@{f.sep}
@end
@end
@end
@end
@end
@end
    );
}

@end
template<typename T> static void ntg_elem_free(T*) {}
static inline void ntg_elem_free(char** value) { if (value && *value) free(*value); }
static inline void ntg_elem_free(ntg_bytes* value) { if (value) free(value->data); }
@for s in structs
static inline void ntg_elem_free(ntg_@{s.name|snake}* value) { ntg_@{s.name|snake}_free(value); }
@end

@for s in structs
extern "C" NTG_C_EXPORT void ntg_@{s.name|snake}_free(ntg_@{s.name|snake}* value) {
    if (!value) {
        return;
    }
@for f in s.fields
@if f.string
    free(value->@{f.name|snake});
@else
@if f.bytes
    free(value->@{f.name|snake});
@else
@if f.vector
    for (size_t i = 0; i < value->@{f.name|snake}_len; ++i) {
        ntg_elem_free(&value->@{f.name|snake}[i]);
    }
    free(value->@{f.name|snake});
@else
@if f.optional
@if f.isstruct
    if (value->@{f.name|snake}) { ntg_@{f.type|base|snake}_free(value->@{f.name|snake}); free(value->@{f.name|snake}); }
@else
    free(value->@{f.name|snake});
@end
@else
@if f.isstruct
    ntg_@{f.type|base|snake}_free(&value->@{f.name|snake});
@end
@end
@end
@end
@end
@end
}

@end
@for me in mapentries
extern "C" NTG_C_EXPORT void @{me.valtl|type|snake}_entry_free(@{me.valtl|type|snake}_entry* entries, size_t len) {
    if (!entries) {
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        ntg_@{me.valtl|base|snake}_free(&entries[i].value);
    }
    free(entries);
}

@end
extern "C" NTG_C_EXPORT void ntg_string_free(char* value) {
    free(value);
}

extern "C" NTG_C_EXPORT void ntg_bytes_free(void* value) {
    free(value);
}

@for c in classes
extern "C" NTG_C_EXPORT ntg_instance* ntg_instance_create(void) {
    auto* inst = new ntg_instance();
    inst->impl = new @{c.cpp}();
@for cb in callbacks
    inst->@{cb.method}_fn = nullptr;
    inst->@{cb.method}_ud = nullptr;
    inst->impl->@{cb.method}([inst](
@for a in cb.cbargs
        @{a.cpptype} @{a.name|snake}@{a.sep}
@end
    ) {
        const auto fn = inst->@{cb.method}_fn;
        if (!fn) {
            return;
        }
@for a in cb.cbargs
@if a.isstruct
        auto _c_@{a.name|snake} = to_c_@{a.type|base}(@{a.name|snake});
@else
@if a.vector
        @{a.type|type|snake}* _c_@{a.name|snake} = nullptr;
        size_t _c_@{a.name|snake}_len = 0;
        ntg_to_c_vector(@{a.name|snake}, &_c_@{a.name|snake}, &_c_@{a.name|snake}_len);
@end
@end
@end
        fn(inst
@for a in cb.cbargs
@if a.bytes
            , @{a.name|snake}.data(), @{a.name|snake}.size()
@else
@if a.string
            , @{a.name|snake}.c_str()
@else
@if a.isstruct
            , _c_@{a.name|snake}
@else
@if a.vector
            , _c_@{a.name|snake}, _c_@{a.name|snake}_len
@else
@if a.scalar
            , @{a.name|snake}
@else
            , to_c_@{a.type|base}(@{a.name|snake})
@end
@end
@end
@end
@end
@end
            , inst->@{cb.method}_ud);
@for a in cb.cbargs
@if a.isstruct
        ntg_@{a.type|base|snake}_free(&_c_@{a.name|snake});
@else
@if a.vector
        for (size_t _i = 0; _i < _c_@{a.name|snake}_len; ++_i) ntg_elem_free(&_c_@{a.name|snake}[_i]);
        free(_c_@{a.name|snake});
@end
@end
@end
    });
@end
    return inst;
}

extern "C" NTG_C_EXPORT void ntg_instance_destroy(ntg_instance* handle) {
    if (!handle) {
        return;
    }
    delete handle->impl;
    delete handle;
}

@for m in c.methods
@if m.iscb
@else
extern "C" NTG_C_EXPORT ntg_result ntg_@{m.name|snake}(
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
) {
@if m.static
@else
    if (!handle || !handle->impl) {
        return NTG_ERR_NULL_POINTER;
    }
@end
    try {
@if m.isvoid
@if m.static
        @{c.cpp}::@{m.name|snake}(
@else
        handle->impl->@{m.name|snake}(
@end
@for p in m.params
@if p.bytes
            ntg_to_cpp_bytes(@{p.name|snake}, @{p.name|snake}_len)@{p.sep}
@else
@if p.string
            @{p.name|snake}@{p.sep}
@else
@if p.vector
            ntg_from_c_vector<@{p.elcpp}>(@{p.name|snake}, @{p.name|snake}_len)@{p.sep}
@else
@if p.scalar
            @{p.name|snake}@{p.sep}
@else
            from_c_@{p.type|base}(@{p.name|snake})@{p.sep}
@end
@end
@end
@end
@end
        );
@else
@if m.static
        auto _r = @{c.cpp}::@{m.name|snake}(
@else
        auto _r = handle->impl->@{m.name|snake}(
@end
@for p in m.params
@if p.bytes
            ntg_to_cpp_bytes(@{p.name|snake}, @{p.name|snake}_len)@{p.sep}
@else
@if p.string
            @{p.name|snake}@{p.sep}
@else
@if p.vector
            ntg_from_c_vector<@{p.elcpp}>(@{p.name|snake}, @{p.name|snake}_len)@{p.sep}
@else
@if p.scalar
            @{p.name|snake}@{p.sep}
@else
            from_c_@{p.type|base}(@{p.name|snake})@{p.sep}
@end
@end
@end
@end
@end
        );
@if m.retstring
        *out = ntg_dup_string(_r);
@else
@if m.retbytes
        ntg_dup_bytes(_r, out, out_len);
@else
@if m.retvector
        ntg_to_c_vector(_r, out, out_len);
@else
@if m.retmap
        ntg_map_to_entries(_r, out, out_len);
@else
@if m.retscalar
        *out = _r;
@else
        *out = to_c_@{m.ret|base}(_r);
@end
@end
@end
@end
@end
@end
        return NTG_OK;
    } HANDLE_EXCEPTIONS
}

@end
@end
@for cb in callbacks
extern "C" NTG_C_EXPORT ntg_result ntg_on_@{cb.name|snake}(ntg_instance* handle, ntg_@{cb.name|snake}_cb callback, void* user_data) {
    if (!handle || !handle->impl) {
        return NTG_ERR_NULL_POINTER;
    }
    handle->@{cb.method}_fn = callback;
    handle->@{cb.method}_ud = user_data;
    return NTG_OK;
}

@end
@end

extern "C" NTG_C_EXPORT void ntg_set_log_callback(ntg_log_cb callback, void* user_data) {
    if (callback == nullptr) {
        ntgcalls::utils::LogSink::register_logger([](const ntgcalls::utils::LogSink::LogMessage&) {});
        return;
    }
    ntgcalls::utils::LogSink::register_logger([callback, user_data](const ntgcalls::utils::LogSink::LogMessage& message) {
        ntg_log_message out;
        out.level = static_cast<ntg_log_level>(message.level);
        out.source = static_cast<ntg_log_source>(message.source);
        out.file = message.file.c_str();
        out.line = message.line;
        out.message = message.message.c_str();
        callback(out, user_data);
    });
}
