<img src="https://raw.githubusercontent.com/pytgcalls/ntgcalls/master/.github/images/banner.png" alt="pytgcalls logo" />
<p align="center">
    <b>A Native Implementation of Telegram Calls in a seamless way.</b>
    <br>
    <a href="https://github.com/pytgcalls/ntgcalls/tree/master/examples">
        Examples
    </a>
    •
    <a href="https://pytgcalls.github.io/">
        Documentation
    </a>
    •
    <a href="https://pypi.org/project/ntgcalls/">
        PyPi
    </a>
    •
    <a href="https://www.npmjs.com/package/ntgcalls">
        npm
    </a>
    •
    <a href="https://crates.io/crates/ntgcalls">
        crates.io
    </a>
    •
    <a href="https://central.sonatype.com/artifact/io.github.pytgcalls/ntgcalls">
        Maven Central
    </a>
    •
    <a href="https://github.com/pytgcalls/ntgcalls/releases">
        Releases
    </a>
    •
    <a href="https://t.me/pytgcallsnews">
        Channel
    </a>
    •
    <a href="https://t.me/pytgcallschat">
        Chat
    </a>
</p>

# NTgCalls [![Release](https://img.shields.io/github/v/release/pytgcalls/ntgcalls?include_prereleases&logo=github&logoColor=%23959DA5&label=release&labelColor=%23282f37)](https://github.com/pytgcalls/ntgcalls/releases) [![PyPI - Downloads](https://img.shields.io/pepy/dt/ntgcalls?logo=python&logoColor=%23959DA5&label=pypi&labelColor=%23282f37&color=%2328A745)](https://pepy.tech/project/ntgcalls) [![npm - Downloads](https://img.shields.io/npm/dm/ntgcalls?logo=npm&logoColor=%23959DA5&label=npm&labelColor=%23282f37&color=%2328A745)](https://www.npmjs.com/package/ntgcalls) [![Crates.io - Downloads](https://img.shields.io/crates/d/ntgcalls?logo=rust&logoColor=%23959DA5&label=crates.io&labelColor=%23282f37&color=%2328A745)](https://crates.io/crates/ntgcalls)

NTgCalls is a lightweight open-source library for media streaming in Telegram calls. Built from scratch in C++ with WebRTC & Boost, it prioritizes accessibility to developers and resource efficiency.

|                                                                                     Powerful                                                                                      |                                                                                            Simple                                                                                            |                                                                                                   Light                                                                                                    |
|:---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
| <img src="https://raw.githubusercontent.com/pytgcalls/ntgcalls/master/.github/images/fast.gif" width=150 alt="Fast Logo"/><br>Built from scratch in C++ using Boost and libwebrtc | <img src="https://raw.githubusercontent.com/pytgcalls/ntgcalls/master/.github/images/simple.gif" width=150 alt="Simple Logo"/><br>Simple Python, Node.js, Rust, GO, C Bindings and Java for Android SDK<br> | <img src="https://raw.githubusercontent.com/pytgcalls/ntgcalls/master/.github/images/light.gif" width=150 alt="Light logo"/><br>We removed anything that could burden the library, keeping every binding a thin layer over the same native core |

## Build Status
| Architecture |                                                                   Windows                                                                   |                                                                Linux                                                                |                                                                  MacOS                                                                  |
|:------------:|:-------------------------------------------------------------------------------------------------------------------------------------------:|:-----------------------------------------------------------------------------------------------------------------------------------:|:---------------------------------------------------------------------------------------------------------------------------------------:|
|    x86_64    |   ![BUILD](https://img.shields.io/badge/build-passing-dark_green?logo=windows11&logoColor=%23959DA5&labelColor=%23282f37&color=%2328A745)   | ![BUILD](https://img.shields.io/badge/build-passing-dark_green?logo=linux&logoColor=%23959DA5&labelColor=%23282f37&color=%2328A745) | ![BUILD](https://img.shields.io/badge/build-unsupported-dark_green?logo=apple&logoColor=%23959DA5&labelColor=%23282f37&color=%23959DA5) |
|    ARM64     | ![BUILD](https://img.shields.io/badge/build-unsupported-dark_green?logo=windows11&logoColor=%23959DA5&labelColor=%23282f37&color=%23959DA5) | ![BUILD](https://img.shields.io/badge/build-passing-dark_green?logo=linux&logoColor=%23959DA5&labelColor=%23282f37&color=%2328A745) |   ![BUILD](https://img.shields.io/badge/build-passing-dark_green?logo=apple&logoColor=%23959DA5&labelColor=%23282f37&color=%2328A745)   |

## Features
- Pre-built binaries for macOS (arm64-v8a), Linux (x86_64, arm64-v8a), Windows (x86_64), and Android (x86, 86_64, arm64-v8a, armv7)
- Call flexibility: Group and private call support
- Media controls: pause/resume and mute/unmute
- Codec compatibility: H.264, HEVC (H.265), VP8, VP9, AV1, AAC, MP3, Opus
- Content sharing: Screen streaming, Microphone and Camera streaming
- Pre-built wheels for Python, npm packages for Node.js, crates for Rust & AAR SDK library for Android

## Installing
Prebuilt packages are published for every supported language, so most users never need to compile anything.

| Language          | Package                                                                                              | Install                          |
|:------------------|:-----------------------------------------------------------------------------------------------------|:---------------------------------|
| Python            | [ntgcalls](https://pypi.org/project/ntgcalls/)                                                        | `pip install ntgcalls`           |
| Node.js           | [ntgcalls](https://www.npmjs.com/package/ntgcalls)                                                    | `npm install ntgcalls`           |
| Rust              | [ntgcalls](https://crates.io/crates/ntgcalls)                                                         | `cargo add ntgcalls`             |
| Java (Android)    | [io.github.pytgcalls:ntgcalls](https://central.sonatype.com/artifact/io.github.pytgcalls/ntgcalls)    | Gradle dependency                |
| C, C++ and Go     | [Release archives](https://github.com/pytgcalls/ntgcalls/releases)                                    | Download the shared or static zip |

## Building from source
Prerequisites are listed in the
[Build Guide](https://pytgcalls.github.io/NTgCalls/Build%20Guide#Installing=Prerequisites).
All commands are run from the root of the repository.

| Target       | Command                                    | Output                                     |
|:-------------|:-------------------------------------------|:-------------------------------------------|
| Python       | `python3 setup.py install`                 | Installed in the current environment       |
| Node.js      | `python3 setup.py build_lib --target=node` | `targets/node/build/Release/ntgcalls.node` |
| C and C++    | `python3 setup.py build_lib`               | `shared-output` and `static-output`        |

The C target builds both the shared and the static library in a single run. Each output directory also
contains an `include` folder with the headers to add to your project.

The Rust bindings do not build the native library: the `ntgcalls-sys` build script downloads the matching
static libraries from the GitHub release for your platform. Set `NTGCALLS_RELEASE_BASE` to build against a
local release instead.

<details>
<summary><b>Using the library from Go</b></summary>

> [!WARNING]
> Static linking for Windows is not supported yet since our library is built with MSVC and Go uses MinGW for static linking.
> More info can be found [here](https://github.com/golang/go/issues/63903)

Go consumes the C bindings through an example project in `./examples/go/`:
1. Download the **shared** or **static** release from the [Releases](https://github.com/pytgcalls/ntgcalls/releases) page
2. Copy `ntgcalls.h` into `./examples/go/ntgcalls/`
3. Copy the remaining files into `./examples/go/`
    * `ntgcalls.dll` or `ntgcalls.lib` on Windows amd64
    * `libntgcalls.so` or `libntgcalls.a` on Linux amd64
    * `libntgcalls.dylib` or `libntgcalls.a` on macOS
4. Run `go build` or `go run .` with `CGO_ENABLED=1`
    * `$env:CGO_ENABLED=1; go run .` on Windows PowerShell
    * `CGO_ENABLED=1 go run .` on UNIX

</details>

## Key Contributors
* <b><a href="https://github.com/Laky-64">@Laky-64</a> (DevOps Engineer, Software Architect, Porting Engineer):</b>
    * Played a crucial role in developing NTgCalls.
    * Created the Python Bindings that made the library accessible to Python developers.
    * Developed the C Bindings, enabling the library's use in various environments.
* <b><a href="https://github.com/dadadani">@dadadani</a> (Senior C++ Developer, Performance engineer):</b>
    * Contributed to setting up CMakeLists and integrating with pybind11,
      greatly simplifying the library's usage for C++ developers.
* <b><a href="https://github.com/kuogi">@kuogi</a> (Senior UI/UX designer, Documenter):</b>
    * As a Senior UI/UX Designer, Kuogi has significantly improved the user interface of our documentation,
      making it more visually appealing and user-friendly.
    * It Has also played a key role in writing and structuring our documentation, ensuring that it is clear,
      informative, and accessible to all users.
* <b><a href="https://github.com/vrumger">@vrumger</a> (Mid-level NodeJS Developer):</b>
    * Avrumy has made important fixes and enhancements to the WebRTC component of the library,
      improving its stability and performance.
* <b><a href="https://github.com/ankit-chaubey">@ankit-chaubey</a> (Rust Developer):</b>
    * Authored the first Rust bindings for NTgCalls and generously transferred the
      <a href="https://crates.io/crates/ntgcalls">ntgcalls</a> crate to the official project.

## Junior Developers
* <b><a href="https://github.com/TuriOG">@TuriOG</a> (Junior Python Developer):</b>
    * Currently working on integrating NTgCalls into <a href="//github.com/pytgcalls/pytgcalls">PyTgCalls</a>, an important step
      in expanding the functionality and usability of the library.
* <b><a href="https://github.com/doggyhaha">@doggyhaha</a> (Junior DevOps, Tester):</b>
    * Performs testing of NTgCalls on Linux to ensure its reliability and compatibility.
    * Specializes in creating and maintaining GitHub Actions, focusing on automation tasks.
* <b><a href="https://github.com/tappo03">@tappo03</a> (Junior Go Developer, Tester):</b>
    * Performs testing of NTgCalls on Windows to ensure its reliability and compatibility.
    * It Is in the process of integrating NTgCalls into a Go wrapper, further enhancing the library's
      versatility and accessibility.

## Special Thanks
* <b><a href="https://github.com/shiguredo">@shiguredo</a>:</b>
  We extend our special thanks to 時雨堂 (shiguredo) for their invaluable assistance in integrating the WebRTC component. Their contributions,
  using the Sora C++ SDK, have been instrumental in enhancing the functionality of our library.

* <b><a href="https://github.com/evgeny-nadymov">@evgeny-nadymov</a>:</b>
  A heartfelt thank you to Evgeny Nadymov for graciously allowing us to use their code from telegram-react.
  His contribution has been pivotal to the success of this project.

* <b><a href="https://github.com/morethanwords">@morethanwords</a>:</b>
  We extend our special thanks to morethanwords for their invaluable help in integrating the connection to WebRTC with Telegram Web K.
  Their assistance has been instrumental in enhancing the functionality of our library.

* <b><a href="https://github.com/MarshalX">@MarshalX</a>:</b> for their generous assistance in answering questions and providing insights regarding WebRTC.

* <b><a href="https://github.com/LyzCoote">@LyzCoote</a>:</b> for providing an ARM64 Server and allowing us to build an image with clang-18 preinstalled on manylinux2014 arm64.

_We would like to extend a special thanks to <b><a href='https://github.com/null-nick'>@null-nick</a></b>
and <b><a href='https://github.com/branchscope'>@branchscope</a></b> for their valuable contributions to the testing phase of the library and to
<b><a href='https://github.com/SadLuffy'>@SadLuffy</a></b> for his assistance as a copywriter.
Their dedication to testing and optimizing the library has been instrumental in its success._

_Additionally, we extend our gratitude to all contributors for their exceptional work in making this project a reality._
