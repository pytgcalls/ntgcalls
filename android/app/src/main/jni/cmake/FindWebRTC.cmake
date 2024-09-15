set(WEBRTC_SRC ${DEPS_DIR}/libwebrtc/src)
set(WEBRTC_INCLUDE ${WEBRTC_SRC}/include)
set(WEBRTC_LIB_DIR ${WEBRTC_SRC}/lib/${ANDROID_ABI})
set(WEBRTC_LIB ${WEBRTC_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}webrtc${CMAKE_STATIC_LIBRARY_SUFFIX})

set(_DIRS
    ${WEBRTC_INCLUDE}
    ${WEBRTC_INCLUDE}/third_party/abseil-cpp
    ${WEBRTC_INCLUDE}/third_party/boringssl/src/include
    ${WEBRTC_INCLUDE}/third_party/libyuv/include
    ${WEBRTC_INCLUDE}/third_party/zlib
)

add_library(WebRTC::webrtc UNKNOWN IMPORTED)
set_target_properties(WebRTC::webrtc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_DIRS}"
        IMPORTED_LOCATION ${WEBRTC_LIB})

file(READ ${WEBRTC_LIB_DIR}/webrtc.ldflags WEBRTC_ANDROID_LDFLAGS)
string(REGEX REPLACE "\n" ";" WEBRTC_ANDROID_LDFLAGS "${WEBRTC_ANDROID_LDFLAGS}")