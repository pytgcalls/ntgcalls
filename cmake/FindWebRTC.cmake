GetProperty("version.webrtc" WEBRTC_REVISION)
string(REGEX MATCH "[0-9]+\\.([0-9]+)" WEBRTC_BRANCH "${WEBRTC_REVISION}")
set(WEBRTC_BRANCH branch-heads/${CMAKE_MATCH_1})
set(WEBRTC_GIT https://github.com/pytgcalls/webrtc-build)
set(WEBRTC_DIR ${DEPS_DIR}/libwebrtc)
set(WEBRTC_SRC ${WEBRTC_DIR}/src)
set(WEBRTC_INCLUDE ${WEBRTC_SRC}/include)
set(WEBRTC_LIB_DIR ${WEBRTC_SRC}/lib)
if (ANDROID)
    set(WEBRTC_LIB_DIR ${WEBRTC_LIB_DIR}/${ANDROID_ABI})
endif ()
set(WEBRTC_LIB ${WEBRTC_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}webrtc${CMAKE_STATIC_LIBRARY_SUFFIX})

if(NOT TARGET WebRTC::webrtc)
    message(STATUS "libwebrtc m${WEBRTC_REVISION}")
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
    elseif (ANDROID)
        set(PLATFORM android)
        set(ARCHIVE_FORMAT .tar.gz)
        set(ARCH ${ANDROID_ABI})
    elseif (LINUX)
        set(PLATFORM ubuntu-22.04)
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
    set(FILE_NAME webrtc.${PLATFORM})
    if (NOT ANDROID)
        set(FILE_NAME ${FILE_NAME}_${ARCH})
    endif ()
    set(FILE_NAME ${FILE_NAME}${ARCHIVE_FORMAT})

    DownloadProject(
        URL ${WEBRTC_GIT}/releases/download/m${WEBRTC_REVISION}/${FILE_NAME}
        DOWNLOAD_DIR ${WEBRTC_DIR}/download
        SOURCE_DIR ${WEBRTC_SRC}
    )

    add_library(WebRTC::webrtc STATIC IMPORTED GLOBAL)

    set(_DIRS
        ${WEBRTC_INCLUDE}
        ${WEBRTC_INCLUDE}/third_party/abseil-cpp
        ${WEBRTC_INCLUDE}/third_party/boringssl/src/include
        ${WEBRTC_INCLUDE}/third_party/libyuv/include
        ${WEBRTC_INCLUDE}/third_party/zlib
    )
    if (MACOS)
        list(APPEND _DIRS
            ${WEBRTC_INCLUDE}/sdk/objc
            ${WEBRTC_INCLUDE}/sdk/objc/base
            ${WEBRTC_INCLUDE}/sdk/objc/components/video_codec
        )
    endif()

    set_target_properties(WebRTC::webrtc PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_DIRS}"
            IMPORTED_LOCATION "${WEBRTC_LIB}")

    file(READ ${WEBRTC_SRC}/VERSIONS WEBRTC_DATA)
    string(REGEX MATCH "WEBRTC_SRC_THIRD_PARTY_LIBCXX_SRC_COMMIT=([^ \n]+)" matched "${WEBRTC_DATA}")
    set(LIBCXX_COMMIT ${CMAKE_MATCH_1})
    string(REGEX MATCH "WEBRTC_SRC_THIRD_PARTY_LIBCXXABI_SRC_COMMIT=([^ \n]+)" matched "${WEBRTC_DATA}")
    set(LIBCXX_ABI_COMMIT ${CMAKE_MATCH_1})
    string(REGEX MATCH "WEBRTC_SRC_BUILDTOOLS_COMMIT=([^ \n]+)" matched "${WEBRTC_DATA}")
    set(BUILDTOOLS_COMMIT ${CMAKE_MATCH_1})
endif ()
