use ntgcalls_sys as sys;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;

pub(crate) trait FromC {
    type C;
    unsafe fn from_c(c: &Self::C) -> Self;
}

pub(crate) trait ToC {
    type C;
    fn to_c(&self, conv: &mut Conv) -> Self::C;
}

#[derive(Default)]
pub(crate) struct Conv {
    keep: Vec<Box<dyn std::any::Any>>,
}

impl Conv {
    pub(crate) fn cstr(&mut self, s: &str) -> *mut c_char {
        let c = CString::new(s).unwrap();
        let ptr = c.as_ptr() as *mut c_char;
        self.keep.push(Box::new(c));
        ptr
    }

    pub(crate) fn boxed<T: 'static>(&mut self, value: T) -> *mut T {
        let mut b = Box::new(value);
        let ptr = &mut *b as *mut T;
        self.keep.push(b);
        ptr
    }

    pub(crate) fn vec<T: 'static>(&mut self, mut value: Vec<T>) -> (*mut T, usize) {
        let ptr = value.as_mut_ptr();
        let len = value.len();
        self.keep.push(Box::new(value));
        (ptr, len)
    }
}

macro_rules! copy_to_c {
    ($($t:ty),* $(,)?) => {
        $(impl $crate::util::ToC for $t {
            type C = $t;
            fn to_c(&self, _conv: &mut $crate::util::Conv) -> $t {
                *self
            }
        })*
    };
}
pub(crate) use copy_to_c;

copy_to_c!(i8, u8, i16, u16, i32, u32, i64, u64, f64, bool);

impl ToC for String {
    type C = *mut c_char;
    fn to_c(&self, conv: &mut Conv) -> *mut c_char {
        conv.cstr(self)
    }
}

impl ToC for Vec<u8> {
    type C = sys::ntg_bytes;
    fn to_c(&self, _conv: &mut Conv) -> sys::ntg_bytes {
        sys::ntg_bytes {
            data: self.as_ptr() as *mut u8,
            len: self.len(),
        }
    }
}

macro_rules! copy_from_c {
    ($($t:ty),* $(,)?) => {
        $(impl $crate::util::FromC for $t {
            type C = $t;
            unsafe fn from_c(c: &$t) -> $t {
                *c
            }
        })*
    };
}
pub(crate) use copy_from_c;

copy_from_c!(i8, u8, i16, u16, i32, u32, i64, u64, f64, bool);

impl FromC for String {
    type C = *mut c_char;
    unsafe fn from_c(c: &*mut c_char) -> String {
        cstr(*c)
    }
}

impl FromC for Vec<u8> {
    type C = sys::ntg_bytes;
    unsafe fn from_c(c: &sys::ntg_bytes) -> Vec<u8> {
        bytes(c.data, c.len)
    }
}

pub(crate) fn cstr(ptr: *const c_char) -> String {
    if ptr.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned()
    }
}

pub(crate) fn take_string(ptr: *mut c_char) -> String {
    let value = cstr(ptr);
    if !ptr.is_null() {
        unsafe { sys::ntg_string_free(ptr) };
    }
    value
}

pub(crate) unsafe fn bytes(ptr: *mut u8, len: usize) -> Vec<u8> {
    if ptr.is_null() {
        Vec::new()
    } else {
        std::slice::from_raw_parts(ptr, len).to_vec()
    }
}

pub(crate) unsafe fn vec_from_c<R: FromC>(ptr: *mut R::C, len: usize) -> Vec<R> {
    if ptr.is_null() {
        Vec::new()
    } else {
        std::slice::from_raw_parts(ptr, len)
            .iter()
            .map(|e| R::from_c(e))
            .collect()
    }
}

pub(crate) unsafe fn opt_from_c<R: FromC>(ptr: *mut R::C) -> Option<R> {
    if ptr.is_null() {
        None
    } else {
        Some(R::from_c(&*ptr))
    }
}
