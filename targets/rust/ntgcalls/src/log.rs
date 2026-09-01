use crate::util::cstr;
use ntgcalls_sys as sys;
use std::os::raw::c_void;

unsafe extern "C" fn trampoline(message: sys::ntg_log_message, _user_data: *mut c_void) {
    let level = match message.level {
        sys::NTG_LOG_ERROR => log::Level::Error,
        sys::NTG_LOG_WARNING => log::Level::Warn,
        sys::NTG_LOG_INFO => log::Level::Info,
        _ => log::Level::Debug,
    };
    let target = if message.source == sys::NTG_LOG_SOURCE_SELF {
        "ntgcalls"
    } else {
        "ntgcalls::webrtc"
    };
    log::log!(target: target, level, "{}:{} {}", cstr(message.file), message.line, cstr(message.message));
}

pub fn init_logging() {
    unsafe { sys::ntg_set_log_callback(Some(trampoline), std::ptr::null_mut()) };
}
