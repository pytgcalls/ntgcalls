use std::env;
use std::fs;
use std::path::PathBuf;

fn shared_name() -> Option<&'static str> {
    let os = env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
    let arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap_or_default();
    match (os.as_str(), arch.as_str()) {
        ("linux", "x86_64") => Some("linux-x86_64"),
        ("linux", "aarch64") => Some("linux-arm64"),
        ("macos", _) => Some("macos-arm64"),
        ("windows", "x86_64") => Some("windows-x86_64"),
        _ => None,
    }
}

fn download_lib(out: &PathBuf) -> PathBuf {
    let name = shared_name().expect("unsupported target platform");
    let version = env::var("CARGO_PKG_VERSION").unwrap();
    let base = env::var("NTGCALLS_RELEASE_BASE")
        .unwrap_or_else(|_| "https://github.com/pytgcalls/ntgcalls/releases/download".into());
    let url = format!("{base}/v{version}/ntgcalls.{name}-static_libs.zip");
    let dir = out.join("ntgcalls-lib");
    if !dir.join("lib").exists() {
        let bytes = ureq::get(&url)
            .call()
            .unwrap_or_else(|e| panic!("failed to fetch {url}: {e}"))
            .into_body()
            .into_with_config()
            .limit(u64::MAX)
            .read_to_vec()
            .expect("read release body");
        let reader = std::io::Cursor::new(bytes);
        let mut zip = zip::ZipArchive::new(reader).expect("open zip");
        fs::create_dir_all(&dir).unwrap();
        zip.extract(&dir).expect("extract zip");
    }
    dir.join("lib")
}

fn lib_dir() -> PathBuf {
    if let Ok(dir) = env::var("NTGCALLS_LIB_DIR") {
        return PathBuf::from(dir);
    }
    let local = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../static-output/lib");
    if local.exists() {
        return local;
    }
    let out = PathBuf::from(env::var("OUT_DIR").unwrap());
    download_lib(&out)
}

fn main() {
    let dir = lib_dir();
    println!("cargo:rerun-if-env-changed=NTGCALLS_LIB_DIR");
    println!("cargo:rerun-if-env-changed=NTGCALLS_DYLIB");
    println!("cargo:rustc-link-search=native={}", dir.display());

    if env::var("NTGCALLS_DYLIB").is_ok() {
        println!("cargo:rustc-link-lib=dylib=ntgcalls");
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", dir.display());
        println!("cargo:rustc-link-arg=-Wl,--allow-shlib-undefined");
        return;
    }

    println!("cargo:rustc-link-lib=static=ntgcalls");

    match env::var("CARGO_CFG_TARGET_OS").as_deref() {
        Ok("linux") => {
            for lib in ["stdc++", "m", "z", "resolv"] {
                println!("cargo:rustc-link-lib=dylib={lib}");
            }
        }
        Ok("macos") => {
            println!("cargo:rustc-link-lib=dylib=c++");
            println!("cargo:rustc-link-lib=dylib=z");
            println!("cargo:rustc-link-lib=framework=Foundation");
            println!("cargo:rustc-link-lib=framework=CoreFoundation");
        }
        Ok("windows") => {
            for lib in [
                "winmm", "ws2_32", "strmiids", "dmoguids", "iphlpapi", "msdmo",
                "secur32", "wmcodecdspuuid", "d3d11", "dxgi", "dwmapi", "shcore",
                "bcrypt", "ntdll", "crypt32", "userenv", "gdi32", "user32",
            ] {
                println!("cargo:rustc-link-lib=dylib={lib}");
            }
        }
        _ => {}
    }
}
