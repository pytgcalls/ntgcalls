set(NTGCALLS_LIB ${CMAKE_SOURCE_DIR}/ntgcalls/${ANDROID_ABI}/${CMAKE_STATIC_LIBRARY_PREFIX}ntgcalls${CMAKE_STATIC_LIBRARY_SUFFIX})
set(WEBRTC_SRC ${DEPS_DIR}/libwebrtc/src)
set(WEBRTC_INCLUDE ${WEBRTC_SRC}/include)
set(WEBRTC_LIB_DIR ${WEBRTC_SRC}/lib/${ANDROID_ABI})

add_library(NTgCalls::ntgcalls STATIC IMPORTED)

set(_DIRS
    ${NTGCALLS_INCLUDE_DIR}
    ${WRTC_INCLUDE_DIR}
    ${DEPS_DIR}/boost/src/include
    ${DEPS_DIR}/ffmpeg/src/include
    ${WEBRTC_INCLUDE}
    ${WEBRTC_INCLUDE}/third_party/abseil-cpp
    ${WEBRTC_INCLUDE}/third_party/boringssl/src/include
    ${WEBRTC_INCLUDE}/third_party/libyuv/include
    ${WEBRTC_INCLUDE}/third_party/zlib
)

set_target_properties(
    NTgCalls::ntgcalls PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "${BASE_COMPILE_DEFINITIONS}"
    INTERFACE_COMPILE_OPTIONS "${BASE_COMPILE_OPTIONS}"
    INTERFACE_INCLUDE_DIRECTORIES "${_DIRS}"
    IMPORTED_LOCATION "${NTGCALLS_LIB}"
)

file(READ ${WEBRTC_LIB_DIR}/webrtc.ldflags WEBRTC_ANDROID_LDFLAGS)
string(REGEX REPLACE "\n" ";" WEBRTC_ANDROID_LDFLAGS "${WEBRTC_ANDROID_LDFLAGS}")