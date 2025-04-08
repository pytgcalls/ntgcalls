set(MESA_DIR ${deps_loc}/mesa)
set(MESA_SRC ${MESA_DIR}/src)
set(MESA_GIT https://github.com/pytgcalls/mesa)

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

GetProperty("version.mesa" MESA_VERSION)
message(STATUS "mesa v${MESA_VERSION}")

set(FILE_NAME mesa.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})
DownloadProject(
    URL ${MESA_GIT}/releases/download/v${MESA_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${MESA_DIR}/download
    SOURCE_DIR ${MESA_SRC}
)
