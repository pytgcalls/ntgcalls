if (NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." REALPATH)
endif ()
if (NOT DEFINED DEPS_DIR)
    set(DEPS_DIR "${ROOT_DIR}/deps")
endif ()
if (NOT DEFINED NOTICES_OUTPUT)
    set(NOTICES_OUTPUT "${ROOT_DIR}/THIRD_PARTY_NOTICES.md")
endif ()
set(NOTICES_CACHE "${DEPS_DIR}/notices")
set(LGPL_URL https://raw.githubusercontent.com/FFmpeg/FFmpeg/n{version}/COPYING.LGPLv2.1)

set_property(GLOBAL PROPERTY notices.table "")
set_property(GLOBAL PROPERTY notices.texts "")

function(fetch_notice url result)
    string(MD5 hash "${url}")
    set(path "${NOTICES_CACHE}/${hash}.txt")
    if (NOT EXISTS "${path}")
        file(DOWNLOAD "${url}" "${path}" STATUS status)
        list(GET status 0 code)
        if (NOT code EQUAL 0)
            file(REMOVE "${path}")
            message(WARNING "Unable to download ${url}")
            set(${result} "" PARENT_SCOPE)
            return()
        endif ()
    endif ()
    file(READ "${path}" text)
    set(${result} "${text}" PARENT_SCOPE)
endfunction()

function(extract_license_comment text result)
    string(FIND "${text}" "Permission" permission)
    string(SUBSTRING "${text}" 0 ${permission} head)
    string(FIND "${head}" "/*" begin REVERSE)
    string(SUBSTRING "${text}" ${begin} -1 text)
    string(FIND "${text}" "*/" end)
    string(SUBSTRING "${text}" 2 ${end} text)
    string(REGEX REPLACE "\n[ \t]*\\*/?[ ]?" "\n" text "${text}")
    set(${result} "${text}" PARENT_SCOPE)
endfunction()

function(add_notice)
    cmake_parse_arguments(ARG "LGPL;COMMENT" "NAME;VERSION;VERSION_KEY;LICENSE;UPSTREAM;BUILD;PLATFORMS;URL;FILE" "" ${ARGN})
    if (ARG_VERSION_KEY)
        file(STRINGS "${ROOT_DIR}/version.properties" property REGEX "^${ARG_VERSION_KEY}=")
        string(REPLACE "${ARG_VERSION_KEY}=" "" ARG_VERSION "${property}")
    endif ()
    set(build "-")
    if (ARG_BUILD)
        set(build "<${ARG_BUILD}>")
    endif ()
    set(version "${ARG_VERSION}")
    if (NOT version)
        set(version "-")
    endif ()
    set_property(GLOBAL APPEND_STRING PROPERTY notices.table
        "| ${ARG_NAME} | ${version} | ${ARG_LICENSE} | <${ARG_UPSTREAM}> | ${build} | ${ARG_PLATFORMS} |\n"
    )
    if (ARG_LGPL)
        set(text "${ARG_NAME} is licensed under the GNU Lesser General Public License version 2.1 or later.\nThe full text of the LGPL-2.1 is reproduced at the end of this file.")
    elseif (ARG_URL)
        string(REPLACE "{version}" "${ARG_VERSION}" url "${ARG_URL}")
        fetch_notice("${url}" text)
        if (ARG_COMMENT AND text)
            extract_license_comment("${text}" text)
        endif ()
    elseif (ARG_FILE)
        file(READ "${ROOT_DIR}/${ARG_FILE}" text)
    else ()
        return()
    endif ()
    string(STRIP "${text}" text)
    if (text)
        set_property(GLOBAL APPEND_STRING PROPERTY notices.texts "### ${ARG_NAME}\n\n````\n${text}\n````\n\n")
    endif ()
endfunction()

add_notice(NAME "WebRTC" VERSION_KEY version.webrtc LICENSE "BSD-3-Clause"
    UPSTREAM https://webrtc.googlesource.com/src BUILD https://github.com/pytgcalls/webrtc-build
    PLATFORMS "All")
add_notice(NAME "Boost" VERSION_KEY version.boost LICENSE "BSL-1.0"
    UPSTREAM https://github.com/boostorg/boost BUILD https://github.com/pytgcalls/boost
    PLATFORMS "All" URL https://raw.githubusercontent.com/boostorg/boost/boost-{version}/LICENSE_1_0.txt)
add_notice(NAME "LLVM libc++ / libc++abi" LICENSE "Apache-2.0 WITH LLVM-exception"
    UPSTREAM https://github.com/llvm/llvm-project
    PLATFORMS "Linux, macOS, Android")
add_notice(NAME "FFmpeg" VERSION_KEY version.ffmpeg LICENSE "LGPL-2.1-or-later"
    UPSTREAM https://github.com/FFmpeg/FFmpeg BUILD https://github.com/pytgcalls/ffmpeg
    PLATFORMS "All" LGPL)
add_notice(NAME "Opus" VERSION 1.6.1 LICENSE "BSD-3-Clause"
    UPSTREAM https://github.com/xiph/opus BUILD https://github.com/pytgcalls/ffmpeg
    PLATFORMS "All" URL https://raw.githubusercontent.com/xiph/opus/v{version}/COPYING)
add_notice(NAME "nv-codec-headers" VERSION 13.1.15.0 LICENSE "MIT"
    UPSTREAM https://github.com/FFmpeg/nv-codec-headers BUILD https://github.com/pytgcalls/ffmpeg
    PLATFORMS "Linux, Windows" COMMENT
    URL https://raw.githubusercontent.com/FFmpeg/nv-codec-headers/n{version}/include/ffnvcodec/dynlink_loader.h)
add_notice(NAME "libva" VERSION 2.24.1 LICENSE "MIT"
    UPSTREAM https://github.com/intel/libva BUILD https://github.com/pytgcalls/ffmpeg
    PLATFORMS "Linux" URL https://raw.githubusercontent.com/intel/libva/{version}/COPYING)
add_notice(NAME "libvdpau" VERSION 1.5 LICENSE "MIT"
    UPSTREAM https://gitlab.freedesktop.org/vdpau/libvdpau BUILD https://github.com/pytgcalls/ffmpeg
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/vdpau/libvdpau/-/raw/{version}/COPYING)
add_notice(NAME "OpenH264" VERSION_KEY version.openh264 LICENSE "BSD-2-Clause"
    UPSTREAM https://github.com/cisco/openh264 BUILD https://github.com/pytgcalls/openh264
    PLATFORMS "Linux, macOS, Windows" URL https://raw.githubusercontent.com/cisco/openh264/v{version}/LICENSE)
add_notice(NAME "GLib" VERSION_KEY version.glib LICENSE "LGPL-2.1-or-later"
    UPSTREAM https://github.com/GNOME/glib BUILD https://github.com/pytgcalls/glib
    PLATFORMS "Linux" LGPL)
add_notice(NAME "libffi" VERSION 3.5.2 LICENSE "MIT"
    UPSTREAM https://github.com/libffi/libffi BUILD https://github.com/pytgcalls/glib
    PLATFORMS "Linux" URL https://raw.githubusercontent.com/libffi/libffi/v{version}/LICENSE)
add_notice(NAME "PCRE2" VERSION 10.46 LICENSE "BSD-3-Clause WITH PCRE2-exception"
    UPSTREAM https://github.com/PCRE2Project/pcre2 BUILD https://github.com/pytgcalls/glib
    PLATFORMS "Linux" URL https://raw.githubusercontent.com/PCRE2Project/pcre2/pcre2-{version}/LICENCE.md)
add_notice(NAME "Expat" VERSION 2.8.3 LICENSE "MIT"
    UPSTREAM https://github.com/libexpat/libexpat BUILD https://github.com/pytgcalls/glib
    PLATFORMS "Linux" URL https://raw.githubusercontent.com/libexpat/libexpat/R_2_8_3/expat/COPYING)
add_notice(NAME "Mesa (libgbm)" VERSION_KEY version.mesa LICENSE "MIT"
    UPSTREAM https://gitlab.freedesktop.org/mesa/mesa BUILD https://github.com/pytgcalls/mesa
    PLATFORMS "Linux" COMMENT URL https://gitlab.freedesktop.org/mesa/mesa/-/raw/mesa-{version}/src/gbm/main/gbm.c)
add_notice(NAME "libdrm" VERSION 2.4.134 LICENSE "MIT"
    UPSTREAM https://gitlab.freedesktop.org/mesa/drm BUILD https://github.com/pytgcalls/mesa
    PLATFORMS "Linux" COMMENT URL https://gitlab.freedesktop.org/mesa/drm/-/raw/libdrm-{version}/xf86drm.h)
add_notice(NAME "libX11" VERSION_KEY version.libX11 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libx11 BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libx11/-/raw/libX11-{version}/COPYING)
add_notice(NAME "libxcb" VERSION 1.17.0 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxcb BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxcb/-/raw/libxcb-{version}/COPYING)
add_notice(NAME "libXau" VERSION 1.0.12 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxau BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxau/-/raw/libXau-{version}/COPYING)
add_notice(NAME "libXext" VERSION 1.3.7 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxext BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxext/-/raw/libXext-{version}/COPYING)
add_notice(NAME "libXfixes" VERSION 6.0.2 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxfixes BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxfixes/-/raw/libXfixes-{version}/COPYING)
add_notice(NAME "libXrandr" VERSION 1.5.5 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxrandr BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxrandr/-/raw/libXrandr-{version}/COPYING)
add_notice(NAME "libXrender" VERSION 0.9.12 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxrender BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxrender/-/raw/libXrender-{version}/COPYING)
add_notice(NAME "libXtst" VERSION 1.2.5 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxtst BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxtst/-/raw/libXtst-{version}/COPYING)
add_notice(NAME "libXcomposite" VERSION 0.4.7 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxcomposite BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxcomposite/-/raw/libXcomposite-{version}/COPYING)
add_notice(NAME "libXdamage" VERSION 1.1.7 LICENSE "MIT / X11"
    UPSTREAM https://gitlab.freedesktop.org/xorg/lib/libxdamage BUILD https://github.com/pytgcalls/libx11
    PLATFORMS "Linux" URL https://gitlab.freedesktop.org/xorg/lib/libxdamage/-/raw/libXdamage-{version}/COPYING)
add_notice(NAME "Oboe" LICENSE "Apache-2.0"
    UPSTREAM https://github.com/google/oboe
    PLATFORMS "Android" FILE deps/oboe/LICENSE)
add_notice(NAME "pybind11" LICENSE "BSD-3-Clause"
    UPSTREAM https://github.com/pybind/pybind11
    PLATFORMS "Python" FILE deps/pybind11/LICENSE)
add_notice(NAME "python-webrtc" LICENSE "BSD-3-Clause"
    UPSTREAM https://github.com/MarshalX/python-webrtc
    PLATFORMS "All" URL https://raw.githubusercontent.com/MarshalX/python-webrtc/main/LICENSE.md)
add_notice(NAME "node-webrtc" LICENSE "BSD-2-Clause"
    UPSTREAM https://github.com/node-webrtc/node-webrtc
    PLATFORMS "All" URL https://raw.githubusercontent.com/node-webrtc/node-webrtc/develop/LICENSE.md)

get_property(table GLOBAL PROPERTY notices.table)
get_property(texts GLOBAL PROPERTY notices.texts)
string(CONCAT content [=[
# Third-Party Notices

NTgCalls is licensed under the GNU Lesser General Public License v3.0 (see `LICENSE`).

The prebuilt binaries (Python wheels, npm packages, Android AAR, C shared/static libraries) statically link the third-party software listed below. Each component remains under its own license, reproduced in this file. Which components are included depends on the platform.

## Components

| Component | Version | License | Upstream source | Build scripts and configuration | Platforms |
|---|---|---|---|---|---|
]=] "${table}" [=[

The `pytgcalls/*` repositories contain the exact build scripts, configure flags and patches used to produce the static libraries, and pin the upstream source versions in their `libraries.properties`. The versions used by each NTgCalls release are pinned in [`version.properties`](version.properties).

## LGPL components

FFmpeg and GLib are licensed under the LGPL-2.1-or-later and are statically linked.

- FFmpeg is configured without `--enable-gpl`, `--enable-nonfree` or `--enable-version3`, so no GPL or non-free code is included.
- The complete corresponding source of NTgCalls is available at <https://github.com/pytgcalls/ntgcalls> under the LGPL-3.0. Together with the build scripts listed above, this lets you rebuild and relink NTgCalls against modified versions of FFmpeg or GLib.

## Patent notice

NTgCalls includes H.264 and HEVC codec implementations (OpenH264, FFmpeg). These formats may be covered by patents in some jurisdictions. The OpenH264 library in these binaries is built from source and is **not** the binary distributed by Cisco, so Cisco's patent license for its OpenH264 binaries does not apply to it. You are responsible for obtaining any patent licenses required where you use or distribute these binaries.

## License texts

]=] "${texts}")

if (NOT EXISTS "${DEPS_DIR}/libwebrtc/src/NOTICE")
    if (EXISTS "${NOTICES_OUTPUT}")
        return()
    endif ()
    message(WARNING "WebRTC NOTICE not found, ${NOTICES_OUTPUT} is missing the WebRTC notices")
else ()
    file(READ "${DEPS_DIR}/libwebrtc/src/NOTICE" webrtc_notice)
    string(REPLACE "\n# " "\n### " webrtc_notice "\n${webrtc_notice}")
    string(APPEND content "## WebRTC and its bundled dependencies\n\nThe following notices are taken from the `NOTICE` file shipped with the prebuilt WebRTC library.\n${webrtc_notice}\n")
endif ()

file(STRINGS "${ROOT_DIR}/version.properties" ffmpeg_version REGEX "^version\\.ffmpeg=")
string(REPLACE "version.ffmpeg=" "" ffmpeg_version "${ffmpeg_version}")
string(REPLACE "{version}" "${ffmpeg_version}" lgpl_url "${LGPL_URL}")
fetch_notice("${lgpl_url}" lgpl)
string(STRIP "${lgpl}" lgpl)
string(APPEND content "## GNU Lesser General Public License v2.1\n\n````\n${lgpl}\n````\n")

set(current "")
if (EXISTS "${NOTICES_OUTPUT}")
    file(READ "${NOTICES_OUTPUT}" current)
endif ()
if (NOT current STREQUAL content)
    file(WRITE "${NOTICES_OUTPUT}" "${content}")
endif ()
