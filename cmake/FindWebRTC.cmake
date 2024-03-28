string(REGEX MATCH "m[0-9]+\\.([0-9]+)" WEBRTC_BRANCH "${WEBRTC_REVISION}")
set(WEBRTC_BRANCH branch-heads/${CMAKE_MATCH_1})
set(WEBRTC_GIT https://github.com/shiguredo-webrtc-build/webrtc-build)
set(WEBRTC_DIR ${deps_loc}/libwebrtc)
set(WEBRTC_SRC ${WEBRTC_DIR}/src)
set(WEBRTC_INCLUDE ${WEBRTC_SRC}/include)
set(WEBRTC_LIB ${WEBRTC_SRC}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}webrtc${CMAKE_STATIC_LIBRARY_SUFFIX})
set(WEBRTC_PATCH_FILE modules/audio_device/include/test_audio_device.cc)
set(WEBRTC_PATCH_URL https://webrtc.googlesource.com/src.git/+/refs/${WEBRTC_BRANCH}/${WEBRTC_PATCH_FILE})

if(NOT TARGET WebRTC::webrtc)
    message(STATUS "libwebrtc ${WEBRTC_REVISION}")
    if (WINDOWS)
        set(PLATFORM windows)
        set(ARCHIVE_FORMAT .zip)
        if (WINDOWS_ARM64)
            set(ARCH arm64)
        elseif (WINDOWS_x86_64)
            set(ARCH x86_64)
        else ()
            set(FAILED_CHECK TRUE)
        endif ()
    elseif (MACOS_ARM64)
        set(PLATFORM macos)
        set(ARCHIVE_FORMAT .tar.gz)
        set(ARCH arm64)
    elseif (LINUX)
        set(PLATFORM ubuntu-20.04)
        set(ARCHIVE_FORMAT .tar.gz)
        if (LINUX_x86_64)
            set(ARCH x86_64)
        elseif (LINUX_ARM64)
            set(ARCH armv8)
        else ()
            set(FAILED_CHECK TRUE)
        endif ()
    else ()
        set(FAILED_CHECK TRUE)
    endif ()

    if (FAILED_CHECK)
        message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} with ${CMAKE_HOST_SYSTEM_PROCESSOR} is not supported yet")
    endif ()

    set(FILE_NAME webrtc.${PLATFORM}_${ARCH}${ARCHIVE_FORMAT})

    DownloadProject(
        URL ${WEBRTC_GIT}/releases/download/${WEBRTC_REVISION}/${FILE_NAME}
        DOWNLOAD_DIR ${WEBRTC_DIR}/download
        SOURCE_DIR ${WEBRTC_SRC}
    )

    add_library(WebRTC::webrtc UNKNOWN IMPORTED)

    target_sources(WebRTC::webrtc INTERFACE ${WEBRTC_PATCH_LOCATION})

    set(WEBRTC_INCLUDE
        ${WEBRTC_INCLUDE}
        ${WEBRTC_INCLUDE}/third_party/abseil-cpp
        ${WEBRTC_INCLUDE}/third_party/boringssl/src/include
        ${WEBRTC_INCLUDE}/third_party/libyuv/include
        ${WEBRTC_INCLUDE}/third_party/zlib
    )
    if (MACOS)
        list(APPEND WEBRTC_INCLUDE
            ${WEBRTC_INCLUDE}/sdk/objc
            ${WEBRTC_INCLUDE}/sdk/objc/base
        )
    endif()

    set_target_properties(WebRTC::webrtc PROPERTIES IMPORTED_LOCATION "${WEBRTC_LIB}")

    file(READ ${WEBRTC_SRC}/VERSIONS WEBRTC_DATA)
    string(REGEX MATCH "WEBRTC_SRC_THIRD_PARTY_LIBCXX_SRC_COMMIT=([^ \n]+)" matched "${WEBRTC_DATA}")
    set(LIBCXX_COMMIT ${CMAKE_MATCH_1})
    string(REGEX MATCH "WEBRTC_SRC_THIRD_PARTY_LIBCXXABI_SRC_COMMIT=([^ \n]+)" matched "${WEBRTC_DATA}")
    set(LIBCXX_ABI_COMMIT ${CMAKE_MATCH_1})
endif ()