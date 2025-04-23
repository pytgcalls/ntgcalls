set(MESA_DIR ${deps_loc}/mesa)
set(MESA_SRC ${MESA_DIR}/src)
set(MESA_GIT https://github.com/pytgcalls/mesa)
set(GBM_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}gbm${CMAKE_STATIC_LIBRARY_SUFFIX})
set(DRM_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}drm${CMAKE_STATIC_LIBRARY_SUFFIX})

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

if(NOT TARGET mesa::gbm)
    add_library(mesa::gbm STATIC IMPORTED)
    set_target_properties(mesa::gbm PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${MESA_SRC}/include"
            IMPORTED_LOCATION "${MESA_SRC}/lib/${GBM_LIB}")
    target_link_libraries(mesa::gbm INTERFACE gnu::expat)
endif ()

if(NOT TARGET mesa::drm)
    add_library(mesa::drm STATIC IMPORTED)
    set_target_properties(mesa::drm PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${MESA_SRC}/include"
            IMPORTED_LOCATION "${MESA_SRC}/lib/${DRM_LIB}")
endif ()
