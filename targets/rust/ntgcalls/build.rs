use std::env;

fn main() {
    println!("cargo:rerun-if-env-changed=NTGCALLS_DYLIB");
    println!("cargo:rerun-if-env-changed=NTGCALLS_LIB_DIR");

    if env::var("NTGCALLS_DYLIB").is_err() {
        return;
    }

    if let Ok(dir) = env::var("NTGCALLS_LIB_DIR") {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{dir}");
    }

    if env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("linux") {
        println!("cargo:rustc-link-arg=-Wl,--allow-shlib-undefined");
    }
}
