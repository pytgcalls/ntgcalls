set(AMF_DIR ${deps_loc}/amf)
set(AMF_SRC ${AMF_DIR}/src)
set(AMF_GIT https://github.com/pytgcalls/AMF)
set(AMF_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}amf${CMAKE_STATIC_LIBRARY_SUFFIX})

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
    message(STATUS "FFmpeg is not supported on ${CMAKE_SYSTEM_NAME} with ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    return()
endif ()

GetProperty("version.amf" AMF_VERSION)
message(STATUS "amf v${AMF_VERSION}")

set(FILE_NAME amf.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})
DownloadProject(
    URL ${AMF_GIT}/releases/download/v${AMF_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${AMF_DIR}/download
    SOURCE_DIR ${AMF_SRC}
)

if(NOT TARGET amd::AMF)
    add_library(amd::AMF STATIC IMPORTED)
    set_target_properties(amd::AMF PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${AMF_SRC}/include"
            IMPORTED_LOCATION "${AMF_SRC}/lib/${AMF_LIB}")
endif ()