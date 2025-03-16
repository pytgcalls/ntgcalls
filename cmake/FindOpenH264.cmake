set(OPENH264_DIR ${deps_loc}/openh264)
set(OPENH264_SRC ${OPENH264_DIR}/src)
set(OPENH264_GIT https://github.com/pytgcalls/openh264)
set(OPENH264_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}openh264${CMAKE_STATIC_LIBRARY_SUFFIX})

if(NOT TARGET cisco::OpenH264)
    if (LINUX_ARM64)
        set(PLATFORM linux)
        set(ARCHIVE_FORMAT .tar.gz)
        set(ARCH arm64)
    elseif (LINUX_x86_64)
        set(PLATFORM linux)
        set(ARCHIVE_FORMAT .tar.gz)
        set(ARCH x86_64)
    elseif (MACOS_ARM64)
        set(PLATFORM macos)
        set(ARCHIVE_FORMAT .tar.gz)
        set(ARCH arm64)
    elseif (WINDOWS_x86_64)
        set(PLATFORM windows)
        set(ARCHIVE_FORMAT .zip)
        set(ARCH x86_64)
    else ()
        message(STATUS "OpenH264 is not supported on ${CMAKE_SYSTEM_NAME} with ${CMAKE_HOST_SYSTEM_PROCESSOR}")
        return()
    endif ()

    GetProperty("version.openh264" OPENH264_VERSION)
    message(STATUS "openh264 v${OPENH264_VERSION}")

    set(FILE_NAME openh264.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})

    DownloadProject(
        URL ${OPENH264_GIT}/releases/download/v${OPENH264_VERSION}/${FILE_NAME}
        DOWNLOAD_DIR ${OPENH264_DIR}/download
        SOURCE_DIR ${OPENH264_SRC}
    )
    add_library(cisco::OpenH264 UNKNOWN IMPORTED)

    set_target_properties(cisco::OpenH264 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENH264_SRC}/include"
            IMPORTED_LOCATION "${OPENH264_SRC}/lib/${OPENH264_LIB}")
endif ()