set(FFMPEG_DIR ${deps_loc}/ffmpeg)
set(FFMPEG_SRC ${FFMPEG_DIR}/src)
set(FFMPEG_GIT https://github.com/pytgcalls/ffmpeg)
set(AVCODEC_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}avcodec${CMAKE_STATIC_LIBRARY_SUFFIX})
set(AVFORMAT_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}avformat${CMAKE_STATIC_LIBRARY_SUFFIX})
set(AVUTIL_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}avutil${CMAKE_STATIC_LIBRARY_SUFFIX})

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

GetProperty("version.ffmpeg" FFMPEG_VERSION)
message(STATUS "ffmpeg v${FFMPEG_VERSION}")

set(FILE_NAME ffmpeg.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})
DownloadProject(
    URL ${FFMPEG_GIT}/releases/download/v${FFMPEG_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${FFMPEG_DIR}/download
    SOURCE_DIR ${FFMPEG_SRC}
)

if(NOT TARGET ffmpeg::avcodec)
    add_library(ffmpeg::avcodec UNKNOWN IMPORTED)
    set_target_properties(ffmpeg::avcodec PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SRC}/include"
            IMPORTED_LOCATION "${FFMPEG_SRC}/lib/${AVCODEC_LIB}")
endif ()

if(NOT TARGET ffmpeg::avformat)
    add_library(ffmpeg::avformat UNKNOWN IMPORTED)
    set_target_properties(ffmpeg::avformat PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SRC}/include"
            IMPORTED_LOCATION "${FFMPEG_SRC}/lib/${AVFORMAT_LIB}")
endif ()

if(NOT TARGET ffmpeg::avutil)
    add_library(ffmpeg::avutil UNKNOWN IMPORTED)
    set_target_properties(ffmpeg::avutil PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SRC}/include"
            IMPORTED_LOCATION "${FFMPEG_SRC}/lib/${AVUTIL_LIB}")
endif ()
