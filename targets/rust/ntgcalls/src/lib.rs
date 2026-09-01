mod enums;
mod error;
mod instance;
mod log;
mod structs;
mod util;

pub use enums::*;
pub use error::{Error, ErrorKind, Result};
pub use instance::NTgCalls;
pub use log::init_logging;
pub use structs::*;

use ntgcalls_sys as sys;

pub fn version() -> String {
    util::cstr(unsafe { sys::ntg_get_version() })
}
