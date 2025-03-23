set(GLIB_DIR ${deps_loc}/glib)
set(GLIB_SRC ${GLIB_DIR}/src)
set(GLIB_GIT https://github.com/pytgcalls/glib)

if (LINUX_ARM64)
    set(PLATFORM linux)
    set(ARCHIVE_FORMAT .tar.gz)
    set(ARCH arm64)
elseif (LINUX_x86_64)
    set(PLATFORM linux)
    set(ARCHIVE_FORMAT .tar.gz)
    set(ARCH x86_64)
else ()
    return()
endif ()

GetProperty("version.glib" GLIB_VERSION)
message(STATUS "glib v${GLIB_VERSION}")

set(FILE_NAME glib.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})
DownloadProject(
    URL ${GLIB_GIT}/releases/download/v${GLIB_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${GLIB_DIR}/download
    SOURCE_DIR ${GLIB_SRC}
)