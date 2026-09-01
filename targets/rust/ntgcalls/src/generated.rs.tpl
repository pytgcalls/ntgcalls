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
@typemap string = String
@typemap bytes = Vec<u8>
@typemap media.* = *
@typemap models.* = *
@typemap p2p.* = *
@typemap e2e.* = *
@typemap instances.* = *
@typemap * = *
@file @{config.self_dir}/enums.rs
use ntgcalls_sys as sys;

@for e in enums
@if e.emit
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum @{e.name} {
@for mem in e.members
    @{mem.disp|pascal},
@end
}

impl From<sys::ntg_@{e.name|snake}> for @{e.name} {
    fn from(c: sys::ntg_@{e.name|snake}) -> Self {
        match c {
@for mem in e.members
            sys::ntg_@{e.name|snake}::NTG_@{e.name|snake|upper}_@{mem.disp|snake|upper} => Self::@{mem.disp|pascal},
@end
        }
    }
}

impl From<@{e.name}> for sys::ntg_@{e.name|snake} {
    fn from(v: @{e.name}) -> Self {
        match v {
@for mem in e.members
            @{e.name}::@{mem.disp|pascal} => Self::NTG_@{e.name|snake|upper}_@{mem.disp|snake|upper},
@end
        }
    }
}

impl crate::util::FromC for @{e.name} {
    type C = sys::ntg_@{e.name|snake};
    unsafe fn from_c(c: &Self::C) -> Self {
        (*c).into()
    }
}

impl crate::util::ToC for @{e.name} {
    type C = sys::ntg_@{e.name|snake};
    fn to_c(&self, _conv: &mut crate::util::Conv) -> Self::C {
        (*self).into()
    }
}

@end
@end
@endfile
@file @{config.self_dir}/error.rs
use ntgcalls_sys as sys;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ErrorKind {
@for e in excs
    @{e.name|pascal},
@end
    Unknown(i32),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
    pub kind: ErrorKind,
    pub message: String,
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.message.is_empty() {
            write!(f, "{:?}", self.kind)
        } else {
            write!(f, "{:?}: {}", self.kind, self.message)
        }
    }
}

impl std::error::Error for Error {}

pub type Result<T> = std::result::Result<T, Error>;

pub(crate) fn check(code: sys::ntg_result) -> Result<()> {
    let kind = match code {
        sys::NTG_OK => return Ok(()),
@for e in excs
        sys::NTG_ERR_@{e.name|snake|upper} => ErrorKind::@{e.name|pascal},
@end
        other => ErrorKind::Unknown(other),
    };
    Err(Error {
        kind,
        message: crate::util::cstr(unsafe { sys::ntg_last_error() }),
    })
}
@endfile
@file @{config.self_dir}/structs.rs
#![allow(dead_code)]
use crate::enums::*;
use crate::util::{self, Conv, FromC, ToC};
use ntgcalls_sys as sys;

@for s in structs
#[derive(Debug, Clone)]
pub struct @{s.name} {
@for f in s.fields
@if f.bytes
    pub @{f.name|snake|reserve}: Vec<u8>,
@else
@if f.string
    pub @{f.name|snake|reserve}: String,
@else
@if f.vector
    pub @{f.name|snake|reserve}: Vec<@{f.type|type}>,
@else
@if f.optional
    pub @{f.name|snake|reserve}: Option<@{f.type|type}>,
@else
    pub @{f.name|snake|reserve}: @{f.type|type},
@end
@end
@end
@end
@end
}

impl FromC for @{s.name} {
    type C = sys::ntg_@{s.name|snake};
    unsafe fn from_c(c: &Self::C) -> Self {
        Self {
@for f in s.fields
@if f.bytes
            @{f.name|snake|reserve}: util::bytes(c.@{f.name|snake|reserve}, c.@{f.name|snake|reserve}_len),
@else
@if f.string
            @{f.name|snake|reserve}: util::cstr(c.@{f.name|snake|reserve}),
@else
@if f.vector
            @{f.name|snake|reserve}: util::vec_from_c(c.@{f.name|snake|reserve}, c.@{f.name|snake|reserve}_len),
@else
@if f.optional
            @{f.name|snake|reserve}: util::opt_from_c(c.@{f.name|snake|reserve}),
@else
            @{f.name|snake|reserve}: FromC::from_c(&c.@{f.name|snake|reserve}),
@end
@end
@end
@end
@end
        }
    }
}

impl ToC for @{s.name} {
    type C = sys::ntg_@{s.name|snake};
    fn to_c(&self, conv: &mut Conv) -> Self::C {
        let mut out: Self::C = unsafe { std::mem::zeroed() };
@for f in s.fields
@if f.bytes
        out.@{f.name|snake|reserve} = self.@{f.name|snake|reserve}.as_ptr() as *mut u8;
        out.@{f.name|snake|reserve}_len = self.@{f.name|snake|reserve}.len();
@else
@if f.string
        out.@{f.name|snake|reserve} = conv.cstr(&self.@{f.name|snake|reserve});
@else
@if f.vector
        let @{f.name|snake|reserve}_tmp: Vec<_> =
            self.@{f.name|snake|reserve}.iter().map(|e| e.to_c(conv)).collect();
        let (@{f.name|snake|reserve}_ptr, @{f.name|snake|reserve}_len) = conv.vec(@{f.name|snake|reserve}_tmp);
        out.@{f.name|snake|reserve} = @{f.name|snake|reserve}_ptr;
        out.@{f.name|snake|reserve}_len = @{f.name|snake|reserve}_len;
@else
@if f.optional
        out.@{f.name|snake|reserve} = match &self.@{f.name|snake|reserve} {
            Some(v) => {
                let c = v.to_c(conv);
                conv.boxed(c)
            }
            None => std::ptr::null_mut(),
        };
@else
        out.@{f.name|snake|reserve} = self.@{f.name|snake|reserve}.to_c(conv);
@end
@end
@end
@end
@end
        out
    }
}

@end
@endfile
@file @{config.self_dir}/instance.rs
use crate::enums::*;
use crate::error::{check, Result};
use crate::structs::*;
use crate::util::{self, FromC, ToC};
use ntgcalls_sys as sys;
use std::ffi::CString;
use std::os::raw::{c_char, c_void};

@for cb in callbacks
pub type @{cb.name|pascal}Fn = dyn Fn(
@for a in cb.cbargs
@if a.vector
    Vec<@{a.type|type}>@{a.sep}
@else
    @{a.type|type}@{a.sep}
@end
@end
) + Send + Sync;
@end

pub struct NTgCalls {
    handle: *mut sys::ntg_instance,
@for cb in callbacks
    @{cb.method}_cb: Option<Box<Box<@{cb.name|pascal}Fn>>>,
@end
}

unsafe impl Send for NTgCalls {}

struct SendHandle(*mut sys::ntg_instance);
unsafe impl Send for SendHandle {}

impl NTgCalls {
    pub fn new() -> Self {
        Self {
            handle: unsafe { sys::ntg_instance_create() },
@for cb in callbacks
            @{cb.method}_cb: None,
@end
        }
    }

@for c in classes
@for m in c.methods
@if m.iscb
@else
@if m.retvector
@else
@if m.async
    pub async fn @{m.name|snake|reserve}(
@else
    pub fn @{m.name|snake|reserve}(
@end
@if m.static
@else
        &self,
@end
@for p in m.params
@if p.string
@if p.optional
        @{p.name|snake|reserve}: Option<&str>,
@else
        @{p.name|snake|reserve}: &str,
@end
@else
@if p.bytes
@if p.optional
        @{p.name|snake|reserve}: Option<&[u8]>,
@else
        @{p.name|snake|reserve}: &[u8],
@end
@else
@if p.vector
        @{p.name|snake|reserve}: &[@{p.type|type}],
@else
@if p.isstruct
        @{p.name|snake|reserve}: &@{p.type|type},
@else
        @{p.name|snake|reserve}: @{p.type|type},
@end
@end
@end
@end
@end
@if m.isvoid
    ) -> Result<()> {
@else
@if m.retstring
    ) -> Result<String> {
@else
@if m.retbytes
    ) -> Result<Vec<u8>> {
@else
@if m.retmap
    ) -> Result<std::collections::HashMap<@{m.retmapkey|type}, @{m.retmapval|type}>> {
@else
    ) -> Result<@{m.ret|type}> {
@end
@end
@end
@end
@if m.async
@for p in m.params
@if p.string
@if p.optional
        let @{p.name|snake|reserve} = @{p.name|snake|reserve}.map(|s| s.to_owned());
@else
        let @{p.name|snake|reserve} = @{p.name|snake|reserve}.to_owned();
@end
@else
@if p.bytes
@if p.optional
        let @{p.name|snake|reserve} = @{p.name|snake|reserve}.map(|b| b.to_vec());
@else
        let @{p.name|snake|reserve} = @{p.name|snake|reserve}.to_vec();
@end
@else
@if p.vector
        let @{p.name|snake|reserve} = @{p.name|snake|reserve}.to_vec();
@else
@if p.isstruct
        let @{p.name|snake|reserve} = @{p.name|snake|reserve}.clone();
@end
@end
@end
@end
@end
        let _sh = SendHandle(self.handle);
        tokio::task::spawn_blocking(move || {
            let _sh = _sh;
            let _handle = _sh.0;
@else
@if m.static
@else
        let _handle = self.handle;
@end
@end
@if m.hasconv
        let mut conv = util::Conv::default();
@end
@for p in m.params
@if p.string
@if p.optional
        let @{p.name|snake|reserve}_c = @{p.name|snake|reserve}.map(|s| CString::new(s).unwrap());
        let @{p.name|snake|reserve}_ptr = @{p.name|snake|reserve}_c.as_ref().map_or(std::ptr::null(), |c| c.as_ptr());
@else
        let @{p.name|snake|reserve}_c = CString::new(@{p.name|snake|reserve}).unwrap();
        let @{p.name|snake|reserve}_ptr = @{p.name|snake|reserve}_c.as_ptr();
@end
@end
@end
@for p in m.params
@if p.bytes
@if p.optional
        let @{p.name|snake|reserve}_ptr = @{p.name|snake|reserve}.as_ref().map_or(std::ptr::null(), |b| b.as_ptr());
        let @{p.name|snake|reserve}_len = @{p.name|snake|reserve}.as_ref().map_or(0, |b| b.len());
@else
        let @{p.name|snake|reserve}_ptr = @{p.name|snake|reserve}.as_ptr();
        let @{p.name|snake|reserve}_len = @{p.name|snake|reserve}.len();
@end
@end
@end
@for p in m.params
@if p.isstruct
        let @{p.name|snake|reserve}_c = @{p.name|snake|reserve}.to_c(&mut conv);
@end
@end
@for p in m.params
@if p.vector
        let @{p.name|snake|reserve}_c: Vec<_> = @{p.name|snake|reserve}.iter().map(|e| e.to_c(&mut conv)).collect();
@end
@end
@if m.isvoid
        check(unsafe {
            sys::ntg_@{m.name|snake}(
@if m.static
@else
                _handle,
@end
@for p in m.params
@if p.string
                @{p.name|snake|reserve}_ptr,
@else
@if p.bytes
                @{p.name|snake|reserve}_ptr, @{p.name|snake|reserve}_len,
@else
@if p.vector
                @{p.name|snake|reserve}_c.as_ptr(), @{p.name|snake|reserve}_c.len(),
@else
@if p.isstruct
                @{p.name|snake|reserve}_c,
@else
@if p.scalar
                @{p.name|snake|reserve},
@else
                @{p.name|snake|reserve}.into(),
@end
@end
@end
@end
@end
@end
            )
        })?;
        Ok(())
@else
@if m.retstring
        let mut out = std::ptr::null_mut();
        check(unsafe {
            sys::ntg_@{m.name|snake}(
@if m.static
@else
                _handle,
@end
@for p in m.params
@if p.string
                @{p.name|snake|reserve}_ptr,
@else
@if p.bytes
                @{p.name|snake|reserve}_ptr, @{p.name|snake|reserve}_len,
@else
@if p.vector
                @{p.name|snake|reserve}_c.as_ptr(), @{p.name|snake|reserve}_c.len(),
@else
@if p.isstruct
                @{p.name|snake|reserve}_c,
@else
@if p.scalar
                @{p.name|snake|reserve},
@else
                @{p.name|snake|reserve}.into(),
@end
@end
@end
@end
@end
@end
                &mut out,
            )
        })?;
        Ok(util::take_string(out))
@else
@if m.retbytes
        let mut out = std::ptr::null_mut();
        let mut out_len = 0;
        check(unsafe {
            sys::ntg_@{m.name|snake}(
@if m.static
@else
                _handle,
@end
@for p in m.params
@if p.string
                @{p.name|snake|reserve}_ptr,
@else
@if p.bytes
                @{p.name|snake|reserve}_ptr, @{p.name|snake|reserve}_len,
@else
@if p.vector
                @{p.name|snake|reserve}_c.as_ptr(), @{p.name|snake|reserve}_c.len(),
@else
@if p.isstruct
                @{p.name|snake|reserve}_c,
@else
@if p.scalar
                @{p.name|snake|reserve},
@else
                @{p.name|snake|reserve}.into(),
@end
@end
@end
@end
@end
@end
                &mut out, &mut out_len,
            )
        })?;
        let value = unsafe { util::bytes(out, out_len) };
        unsafe { sys::ntg_bytes_free(out as *mut _) };
        Ok(value)
@else
@if m.retmap
        let mut out = std::ptr::null_mut();
        let mut out_len = 0;
        check(unsafe {
            sys::ntg_@{m.name|snake}(
@if m.static
@else
                _handle,
@end
@for p in m.params
@if p.string
                @{p.name|snake|reserve}_ptr,
@else
@if p.bytes
                @{p.name|snake|reserve}_ptr, @{p.name|snake|reserve}_len,
@else
@if p.vector
                @{p.name|snake|reserve}_c.as_ptr(), @{p.name|snake|reserve}_c.len(),
@else
@if p.isstruct
                @{p.name|snake|reserve}_c,
@else
@if p.scalar
                @{p.name|snake|reserve},
@else
                @{p.name|snake|reserve}.into(),
@end
@end
@end
@end
@end
@end
                &mut out, &mut out_len,
            )
        })?;
        let value = if out.is_null() {
            std::collections::HashMap::new()
        } else {
            let map = unsafe {
                std::slice::from_raw_parts(out, out_len)
                    .iter()
                    .map(|e| (e.key, FromC::from_c(&e.value)))
                    .collect()
            };
            unsafe { sys::ntg_@{m.retmapval|type|snake}_entry_free(out, out_len) };
            map
        };
        Ok(value)
@else
@if m.retscalar
        let mut out = Default::default();
        check(unsafe {
            sys::ntg_@{m.name|snake}(
@if m.static
@else
                _handle,
@end
@for p in m.params
@if p.string
                @{p.name|snake|reserve}_ptr,
@else
@if p.bytes
                @{p.name|snake|reserve}_ptr, @{p.name|snake|reserve}_len,
@else
@if p.vector
                @{p.name|snake|reserve}_c.as_ptr(), @{p.name|snake|reserve}_c.len(),
@else
@if p.isstruct
                @{p.name|snake|reserve}_c,
@else
@if p.scalar
                @{p.name|snake|reserve},
@else
                @{p.name|snake|reserve}.into(),
@end
@end
@end
@end
@end
@end
                &mut out,
            )
        })?;
        Ok(out)
@else
        let mut out = unsafe { std::mem::zeroed() };
        check(unsafe {
            sys::ntg_@{m.name|snake}(
@if m.static
@else
                _handle,
@end
@for p in m.params
@if p.string
                @{p.name|snake|reserve}_ptr,
@else
@if p.bytes
                @{p.name|snake|reserve}_ptr, @{p.name|snake|reserve}_len,
@else
@if p.vector
                @{p.name|snake|reserve}_c.as_ptr(), @{p.name|snake|reserve}_c.len(),
@else
@if p.isstruct
                @{p.name|snake|reserve}_c,
@else
@if p.scalar
                @{p.name|snake|reserve},
@else
                @{p.name|snake|reserve}.into(),
@end
@end
@end
@end
@end
@end
                &mut out,
            )
        })?;
@if m.retstruct
        let value = unsafe { @{m.ret|type}::from_c(&out) };
        unsafe { sys::ntg_@{m.ret|type|snake}_free(&mut out) };
        Ok(value)
@else
        Ok(out.into())
@end
@end
@end
@end
@end
@end
@if m.async
        })
        .await
        .expect("ntgcalls blocking task panicked")
@end
    }

@end
@end
@end
@end
@for cb in callbacks
    pub fn @{cb.method}(
        &mut self,
        callback: impl Fn(
@for a in cb.cbargs
@if a.vector
            Vec<@{a.type|type}>@{a.sep}
@else
            @{a.type|type}@{a.sep}
@end
@end
        ) + Send + Sync + 'static,
    ) {
        let boxed: Box<Box<@{cb.name|pascal}Fn>> = Box::new(Box::new(callback));
        let user_data = (&*boxed as *const Box<@{cb.name|pascal}Fn>) as *mut c_void;
        unsafe {
            sys::ntg_on_@{cb.name|snake}(self.handle, Some(@{cb.name|snake}_trampoline), user_data);
        }
        self.@{cb.method}_cb = Some(boxed);
    }

@end
}

impl Drop for NTgCalls {
    fn drop(&mut self) {
        unsafe { sys::ntg_instance_destroy(self.handle) };
    }
}

impl Default for NTgCalls {
    fn default() -> Self {
        Self::new()
    }
}

@for cb in callbacks
unsafe extern "C" fn @{cb.name|snake}_trampoline(
    _handle: *mut sys::ntg_instance,
@for a in cb.cbargs
@if a.bytes
    @{a.name|snake|reserve}: *const u8, @{a.name|snake|reserve}_len: usize,
@else
@if a.string
    @{a.name|snake|reserve}: *const c_char,
@else
@if a.vector
    @{a.name|snake|reserve}: *const sys::ntg_@{a.type|type|snake}, @{a.name|snake|reserve}_len: usize,
@else
@if a.scalar
    @{a.name|snake|reserve}: @{a.type|type},
@else
    @{a.name|snake|reserve}: sys::ntg_@{a.type|type|snake},
@end
@end
@end
@end
@end
    user_data: *mut c_void,
) {
    let callback = &*(user_data as *const Box<@{cb.name|pascal}Fn>);
@for a in cb.cbargs
@if a.bytes
    let @{a.name|snake|reserve} = util::bytes(@{a.name|snake|reserve} as *mut u8, @{a.name|snake|reserve}_len);
@else
@if a.string
    let @{a.name|snake|reserve} = util::cstr(@{a.name|snake|reserve});
@else
@if a.vector
    let @{a.name|snake|reserve} = util::vec_from_c(@{a.name|snake|reserve} as *mut _, @{a.name|snake|reserve}_len);
@else
@if a.isstruct
    let @{a.name|snake|reserve} = @{a.type|type}::from_c(&@{a.name|snake|reserve});
@else
@if a.scalar
@else
    let @{a.name|snake|reserve}: @{a.type|type} = @{a.name|snake|reserve}.into();
@end
@end
@end
@end
@end
@end
    callback(
@for a in cb.cbargs
        @{a.name|snake|reserve}@{a.sep}
@end
    );
}

@end
@endfile
