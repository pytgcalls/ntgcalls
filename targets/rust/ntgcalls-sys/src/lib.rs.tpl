@typemap Vector<bytes> = ntg_bytes
@typemap Vector<*> = *
@typemap long = i64
@typemap ulong = u64
@typemap int = i32
@typemap uint = u32
@typemap int8 = i8
@typemap uint8 = u8
@typemap int16 = i16
@typemap uint16 = u16
@typemap bool = bool
@typemap double = f64
@typemap string = *mut c_char
@typemap bytes = u8
@typemap Void = ()
@typemap media.* = ntg_*
@typemap models.* = ntg_*
@typemap p2p.* = ntg_*
@typemap e2e.* = ntg_*
@typemap instances.* = ntg_*
@typemap * = ntg_*
@config reserve_prefix = r#
@config reserved = as break const continue dyn else enum extern false fn for if impl in let loop match mod move mut pub ref return static struct trait true type union unsafe use where while async await box do final macro override priv typeof unsized virtual yield
@out @{config.self_dir}/lib.rs
#![allow(non_camel_case_types, non_upper_case_globals, non_snake_case, dead_code)]

use std::os::raw::{c_char, c_int, c_void};

#[repr(C)]
pub struct ntg_bytes {
    pub data: *mut u8,
    pub len: usize,
}

pub type ntg_result = c_int;
pub const NTG_OK: ntg_result = 0;
pub const NTG_ERR_UNKNOWN: ntg_result = -1;
pub const NTG_ERR_NULL_POINTER: ntg_result = -2;
@for e in excs
pub const NTG_ERR_@{e.name|snake|upper}: ntg_result = @{e.code};
@end

pub type ntg_log_level = c_int;
pub const NTG_LOG_DEBUG: ntg_log_level = 1;
pub const NTG_LOG_INFO: ntg_log_level = 2;
pub const NTG_LOG_WARNING: ntg_log_level = 4;
pub const NTG_LOG_ERROR: ntg_log_level = 8;

pub type ntg_log_source = c_int;
pub const NTG_LOG_SOURCE_WEBRTC: ntg_log_source = 1;
pub const NTG_LOG_SOURCE_SELF: ntg_log_source = 2;

#[repr(C)]
pub struct ntg_log_message {
    pub level: ntg_log_level,
    pub source: ntg_log_source,
    pub file: *const c_char,
    pub line: u32,
    pub message: *const c_char,
}

pub type ntg_log_cb = Option<unsafe extern "C" fn(message: ntg_log_message, user_data: *mut c_void)>;

@for e in enums
@if e.emit
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ntg_@{e.name|snake} {
@for mem in e.members
    NTG_@{e.name|snake|upper}_@{mem.disp|snake|upper},
@end
}

@end
@end
@for s in structs
#[repr(C)]
pub struct ntg_@{s.name|snake} {
@for f in s.fields
@if f.bytes
    pub @{f.name|snake|reserve}: *mut u8,
    pub @{f.name|snake|reserve}_len: usize,
@else
@if f.string
    pub @{f.name|snake|reserve}: *mut c_char,
@else
@if f.vector
    pub @{f.name|snake|reserve}: *mut @{f.type|type|snake},
    pub @{f.name|snake|reserve}_len: usize,
@else
@if f.optional
    pub @{f.name|snake|reserve}: *mut @{f.type|type|snake},
@else
    pub @{f.name|snake|reserve}: @{f.type|type|snake},
@end
@end
@end
@end
@end
}

@end
@for me in mapentries
#[repr(C)]
pub struct @{me.valtl|type|snake}_entry {
    pub key: @{me.keytl|type|snake},
    pub value: @{me.valtl|type|snake},
}

@end
#[repr(C)]
pub struct ntg_instance {
    _private: [u8; 0],
}

@for cb in callbacks
pub type ntg_@{cb.name|snake}_cb = Option<unsafe extern "C" fn(
    handle: *mut ntg_instance,
@for a in cb.cbargs
@if a.bytes
    @{a.name|snake|reserve}: *const u8, @{a.name|snake|reserve}_len: usize,
@else
@if a.string
    @{a.name|snake|reserve}: *const c_char,
@else
@if a.vector
    @{a.name|snake|reserve}: *const @{a.type|type|snake}, @{a.name|snake|reserve}_len: usize,
@else
    @{a.name|snake|reserve}: @{a.type|type|snake},
@end
@end
@end
@end
    user_data: *mut c_void,
)>;

@end
extern "C" {
    pub fn ntg_get_version() -> *const c_char;
    pub fn ntg_last_error() -> *const c_char;
    pub fn ntg_set_log_callback(callback: ntg_log_cb, user_data: *mut c_void);
    pub fn ntg_instance_create() -> *mut ntg_instance;
    pub fn ntg_instance_destroy(handle: *mut ntg_instance);
    pub fn ntg_string_free(value: *mut c_char);
    pub fn ntg_bytes_free(value: *mut c_void);
@for s in structs
    pub fn ntg_@{s.name|snake}_free(value: *mut ntg_@{s.name|snake});
@end
@for me in mapentries
    pub fn @{me.valtl|type|snake}_entry_free(entries: *mut @{me.valtl|type|snake}_entry, len: usize);
@end

@for c in classes
@for m in c.methods
@if m.iscb
@else
    pub fn ntg_@{m.name|snake}(
@if m.static
@else
        handle: *mut ntg_instance,
@end
@for p in m.params
@if p.bytes
        @{p.name|snake|reserve}: *const u8, @{p.name|snake|reserve}_len: usize,
@else
@if p.string
        @{p.name|snake|reserve}: *const c_char,
@else
@if p.vector
        @{p.name|snake|reserve}: *const @{p.type|type|snake}, @{p.name|snake|reserve}_len: usize,
@else
@if p.optional
        @{p.name|snake|reserve}: *const @{p.type|type|snake},
@else
        @{p.name|snake|reserve}: @{p.type|type|snake},
@end
@end
@end
@end
@end
@if m.isvoid
@else
@if m.retstring
        out: *mut *mut c_char,
@else
@if m.retbytes
        out: *mut *mut u8, out_len: *mut usize,
@else
@if m.retvector
        out: *mut *mut @{m.ret|type|snake}, out_len: *mut usize,
@else
@if m.retmap
        out: *mut *mut @{m.retmapval|type|snake}_entry, out_len: *mut usize,
@else
        out: *mut @{m.ret|type|snake},
@end
@end
@end
@end
@end
    ) -> ntg_result;
@end
@end
@end

@for cb in callbacks
    pub fn ntg_on_@{cb.name|snake}(handle: *mut ntg_instance, callback: ntg_@{cb.name|snake}_cb, user_data: *mut c_void) -> ntg_result;
@end
}

#[cfg(glibc_resolv_compat)]
mod glibc_resolv_compat {
    use std::os::raw::{c_char, c_int, c_uchar, c_void};

    extern "C" {
        fn dn_expand(msg: *const c_uchar, eom: *const c_uchar, src: *const c_uchar, dst: *mut c_char, dstsiz: c_int) -> c_int;
        fn res_nquery(statp: *mut c_void, dname: *const c_char, class_: c_int, type_: c_int, answer: *mut c_uchar, anslen: c_int) -> c_int;
    }

    #[no_mangle]
    pub unsafe extern "C" fn __dn_expand(msg: *const c_uchar, eom: *const c_uchar, src: *const c_uchar, dst: *mut c_char, dstsiz: c_int) -> c_int {
        dn_expand(msg, eom, src, dst, dstsiz)
    }

    #[no_mangle]
    pub unsafe extern "C" fn __res_nquery(statp: *mut c_void, dname: *const c_char, class_: c_int, type_: c_int, answer: *mut c_uchar, anslen: c_int) -> c_int {
        res_nquery(statp, dname, class_, type_, answer, anslen)
    }
}
