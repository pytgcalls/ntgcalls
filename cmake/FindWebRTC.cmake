GetProperty("version.webrtc" WEBRTC_REVISION)
string(REGEX MATCH "[0-9]+\\.([0-9]+)" WEBRTC_BRANCH "${WEBRTC_REVISION}")
set(WEBRTC_BRANCH branch-heads/${CMAKE_MATCH_1})
set(WEBRTC_GIT https://github.com/pytgcalls/webrtc-build)
set(WEBRTC_DIR ${deps_loc}/libwebrtc)
set(WEBRTC_SRC ${WEBRTC_DIR}/src)
set(WEBRTC_INCLUDE ${WEBRTC_SRC}/include)
set(WEBRTC_LIB_DIR ${WEBRTC_SRC}/lib)
if (ANDROID)
    set(WEBRTC_LIB_DIR ${WEBRTC_LIB_DIR}/${ANDROID_ABI})
endif ()
set(WEBRTC_LIB ${WEBRTC_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}webrtc${CMAKE_STATIC_LIBRARY_SUFFIX})
set(WEBRTC_PATCH_FILE modules/audio_device/include/test_audio_device.cc)
set(WEBRTC_PATCH_URL https://webrtc.googlesource.com/src.git/+/refs/${WEBRTC_BRANCH}/${WEBRTC_PATCH_FILE})

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

    add_library(WebRTC::webrtc UNKNOWN IMPORTED)

    target_sources(WebRTC::webrtc INTERFACE ${WEBRTC_PATCH_LOCATION})

    set(_DIRS
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

    if (ANDROID)
        set(WEBRTC_LD_FLAGS ${WEBRTC_LIB_DIR}/webrtc.ldflags)
        if (NOT EXISTS ${WEBRTC_LD_FLAGS})
            set(READ_ELF_BIN "${NDK_SRC}/toolchains/llvm/prebuilt")
            file(GLOB READ_ELF_BIN ${READ_ELF_BIN}/*)
            set(READ_ELF_BIN "${READ_ELF_BIN}/bin/llvm-readelf")
            execute_process(
                COMMAND ${READ_ELF_BIN} -Ws ${WEBRTC_LIB}
                OUTPUT_VARIABLE ELF_DUMP
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            string(REGEX REPLACE "\n" ";" ELF_DUMP "${ELF_DUMP}")
            set(LD_FLAGS)
            foreach(line ${ELF_DUMP})
                if (line MATCHES "Java_org_webrtc_")
                    string(REGEX REPLACE " +" ";" line "${line}")
                    list(GET line 8 func)
                    list(APPEND LD_FLAGS "-Wl,--undefined=${func}")
                endif ()
            endforeach ()
            string(REGEX REPLACE ";" "\n" LD_FLAGS "${LD_FLAGS}")
            file(WRITE ${WEBRTC_LD_FLAGS} "${LD_FLAGS}")
        endif ()
    endif ()
endif ()