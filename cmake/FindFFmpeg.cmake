set(FFMPEG_DIR ${deps_loc}/ffmpeg)
set(FFMPEG_SRC ${FFMPEG_DIR}/src)
set(FFMPEG_GIT https://github.com/pytgcalls/ffmpeg)
set(FFMPEG_LIB_DIR ${FFMPEG_SRC}/lib)
if (ANDROID)
    set(FFMPEG_LIB_DIR ${FFMPEG_LIB_DIR}/${ANDROID_ABI})
endif ()
set(AVCODEC_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}avcodec${CMAKE_STATIC_LIBRARY_SUFFIX})
set(AVFORMAT_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}avformat${CMAKE_STATIC_LIBRARY_SUFFIX})
set(AVUTIL_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}avutil${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SWRESAMPLE_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}swresample${CMAKE_STATIC_LIBRARY_SUFFIX})
set(VA_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}va${CMAKE_STATIC_LIBRARY_SUFFIX})
set(VA_DRM_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}va-drm${CMAKE_STATIC_LIBRARY_SUFFIX})
set(VA_X11_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}va-x11${CMAKE_STATIC_LIBRARY_SUFFIX})
set(VDPAU_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}vdpau${CMAKE_STATIC_LIBRARY_SUFFIX})

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
elseif (ANDROID)
    set(PLATFORM android)
    set(ARCHIVE_FORMAT .tar.gz)
    set(ARCH ${ANDROID_ABI})
else ()
    message(STATUS "FFmpeg is not supported on ${CMAKE_SYSTEM_NAME} with ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    return()
endif ()

GetProperty("version.ffmpeg" FFMPEG_VERSION)
message(STATUS "ffmpeg v${FFMPEG_VERSION}")

set(FILE_NAME ffmpeg.${PLATFORM})
if (NOT ANDROID)
    set(FILE_NAME ${FILE_NAME}-${ARCH})
endif ()
set(FILE_NAME ${FILE_NAME}${ARCHIVE_FORMAT})
DownloadProject(
    URL ${FFMPEG_GIT}/releases/download/v${FFMPEG_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${FFMPEG_DIR}/download
    SOURCE_DIR ${FFMPEG_SRC}
)

if (LINUX)
    if (NOT TARGET intel::va-drm)
        add_library(intel::va-drm STATIC IMPORTED)
        set_target_properties(intel::va-drm PROPERTIES
                IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${VA_DRM_LIB}")
    endif ()

    if (LINUX_x86_64)
        if (NOT TARGET intel::va-x11)
            add_library(intel::va-x11 STATIC IMPORTED)
            set_target_properties(intel::va-x11 PROPERTIES
                    IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${VA_X11_LIB}")
        endif ()
    endif ()

    if (NOT TARGET intel::va)
        add_library(intel::va STATIC IMPORTED)
        set_target_properties(intel::va PROPERTIES
                IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${VA_LIB}")
    endif ()

    if (NOT TARGET mesa::vdpau)
        add_library(mesa::vdpau STATIC IMPORTED)
        set_target_properties(mesa::vdpau PROPERTIES
                IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${VDPAU_LIB}")
    endif ()
endif ()

if(NOT TARGET ffmpeg::avcodec)
    add_library(ffmpeg::avcodec STATIC IMPORTED)
    set_target_properties(ffmpeg::avcodec PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SRC}/include"
            IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${AVCODEC_LIB}")

    if (IS_LINUX)
        set_target_properties(ffmpeg::avcodec PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES "-Wl,-Bsymbolic")
    endif ()
endif ()

if(NOT TARGET ffmpeg::avformat)
    add_library(ffmpeg::avformat STATIC IMPORTED)
    set_target_properties(ffmpeg::avformat PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SRC}/include"
            IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${AVFORMAT_LIB}")
    target_link_libraries(ffmpeg::avformat INTERFACE ffmpeg::avcodec)

    if (IS_LINUX)
        set_target_properties(ffmpeg::avformat PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES "-Wl,-Bsymbolic")
    endif ()
endif ()

if(NOT TARGET ffmpeg::avutil)
    add_library(ffmpeg::avutil STATIC IMPORTED)
    set_target_properties(ffmpeg::avutil PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SRC}/include"
            IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${AVUTIL_LIB}")
    if (LINUX AND NOT ANDROID)
        target_link_libraries(ffmpeg::avutil INTERFACE xorg::X11 mesa::vdpau intel::va intel::va-drm)
        if (LINUX_x86_64)
            target_link_libraries(ffmpeg::avutil INTERFACE intel::va-x11)
        else ()
            target_link_libraries(ffmpeg::avutil INTERFACE mesa::drm)
        endif ()
        set_target_properties(ffmpeg::avutil PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES "-Wl,-Bsymbolic")
    endif ()
endif ()

if (NOT TARGET ffmpeg::swresample)
    add_library(ffmpeg::swresample STATIC IMPORTED)
    set_target_properties(ffmpeg::swresample PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SRC}/include"
            IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${SWRESAMPLE_LIB}")

    if (IS_LINUX)
        set_target_properties(ffmpeg::swresample PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES "-Wl,-Bsymbolic")
    endif ()
endif ()
