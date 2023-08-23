set(WEBRTC_DIR ${deps_loc}/libwebrtc)
set(WEBRTC_SRC ${WEBRTC_DIR}/src)
set(WEBRTC_INCLUDE ${WEBRTC_SRC}/include)
set(WEBRTC_LIB ${WEBRTC_SRC}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}webrtc${CMAKE_STATIC_LIBRARY_SUFFIX})

if(NOT TARGET WebRTC::webrtc)
    if (WIN32)
        set(PLATFORM windows)
        set(ARCHIVE_FORMAT .zip)
        if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "ARM64")
            set(ARCH arm64)
        elseif (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "AMD64")
            set(ARCH x86_64)
        else ()
            set(FAILED_ARCH TRUE)
        endif ()
    elseif (MACOS)
        set(PLATFORM macos)
        set(ARCHIVE_FORMAT .tar.gz)
        if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(ARCH arm64)
        else ()
            set(FAILED_ARCH TRUE)
        endif ()
    elseif (LINUX)
        set(PLATFORM ubuntu-20.04)
        set(ARCHIVE_FORMAT .tar.gz)
        if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(ARCH x86_64)
        else ()
            set(FAILED_ARCH TRUE)
        endif ()
    else ()
        set(FAILED_OS TRUE)
    endif ()

    if (FAILED_ARCH)
        message(FATAL_ERROR "${CMAKE_HOST_SYSTEM_PROCESSOR} on ${CMAKE_SYSTEM_NAME} is not supported yet")
    elseif (FAILED_OS)
        message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported yet")
    endif ()

    set(GIT_PROJECT https://github.com/shiguredo-webrtc-build/webrtc-build)
    set(FILE_NAME webrtc.${PLATFORM}_${ARCH}${ARCHIVE_FORMAT})

    set(CMAKE_EXTERNAL_PROJECT_IGNORE_CMAKE_FILES TRUE)

    ExternalProject_Add(
        project_libwebrtc

        URL ${GIT_PROJECT}/releases/download/${WEBRTC_REVISION}/${FILE_NAME}

        PREFIX ${WEBRTC_DIR}/prefix
        DOWNLOAD_DIR ${WEBRTC_DIR}/download
        SOURCE_DIR ${WEBRTC_SRC}
        TMP_DIR ${WEBRTC_DIR}/tmp
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
    )

    ExternalProject_Add_StepTargets(project_libwebrtc install)

    add_library(WebRTC::webrtc UNKNOWN IMPORTED)
    add_dependencies(WebRTC::webrtc project_libwebrtc)

    set(WEBRTC_INCLUDE
        ${WEBRTC_INCLUDE}
        ${WEBRTC_INCLUDE}/third_party/abseil-cpp
        ${WEBRTC_INCLUDE}/third_party/boringssl/src/include
        ${WEBRTC_INCLUDE}/third_party/libyuv/include
        ${WEBRTC_INCLUDE}/third_party/zlib
    )
    if (MACOS)
        list(APPEND WEBRTC_INCLUDE
            ${WebRTC_INCLUDE_DIR}/sdk/objc
            ${WebRTC_INCLUDE_DIR}/sdk/objc/base
        )
    endif()

    set_target_properties(WebRTC::webrtc PROPERTIES IMPORTED_LOCATION "${WEBRTC_LIB}")
endif ()