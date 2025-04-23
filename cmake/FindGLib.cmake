set(GLIB_DIR ${deps_loc}/glib)
set(GLIB_SRC ${GLIB_DIR}/src)
set(GLIB_GIT https://github.com/pytgcalls/glib)
set(GIO_2_0_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}gio-2.0${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GLIB_2_0_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}glib-2.0${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GOBJECT_2_0_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}gobject-2.0${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GMODULE_2_0_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}gmodule-2.0${CMAKE_STATIC_LIBRARY_SUFFIX})
set(FFI_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}ffi${CMAKE_STATIC_LIBRARY_SUFFIX})
set(EXPAT_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX})
set(PCRE2_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}pcre2-8${CMAKE_STATIC_LIBRARY_SUFFIX})

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

if(NOT TARGET gnu::expat)
    add_library(gnu::expat STATIC IMPORTED)
    set_target_properties(gnu::expat PROPERTIES
            IMPORTED_LOCATION "${GLIB_SRC}/lib/${EXPAT_LIB}")
endif ()

if(NOT TARGET gnu::libffi)
    add_library(gnu::libffi STATIC IMPORTED)
    set_target_properties(gnu::libffi PROPERTIES
            IMPORTED_LOCATION "${GLIB_SRC}/lib/${FFI_LIB}")
endif ()

if(NOT TARGET gnu::pcre2)
    add_library(gnu::pcre2 STATIC IMPORTED)
    set_target_properties(gnu::pcre2 PROPERTIES
            IMPORTED_LOCATION "${GLIB_SRC}/lib/${PCRE2_LIB}")
endif ()

if(NOT TARGET gnome::gmodule-2.0)
    add_library(gnome::gmodule-2.0 STATIC IMPORTED)
    set_target_properties(gnome::gmodule-2.0 PROPERTIES
            IMPORTED_LOCATION "${GLIB_SRC}/lib/${GMODULE_2_0_LIB}")
endif ()

if(NOT TARGET gnome::glib-2.0)
    add_library(gnome::glib-2.0 STATIC IMPORTED)
    set_target_properties(gnome::glib-2.0 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GLIB_SRC}/include/glib-2.0"
            IMPORTED_LOCATION "${GLIB_SRC}/lib/${GLIB_2_0_LIB}")
    target_link_libraries(gnome::glib-2.0 INTERFACE gnu::pcre2)
endif ()

if(NOT TARGET gnome::gio-2.0)
    add_library(gnome::gio-2.0 STATIC IMPORTED)
    set_target_properties(gnome::gio-2.0 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GLIB_SRC}/include/glib-2.0"
            IMPORTED_LOCATION "${GLIB_SRC}/lib/${GIO_2_0_LIB}")
    target_link_libraries(gnome::gio-2.0 INTERFACE gnome::gmodule-2.0)
endif ()

if(NOT TARGET gnome::gobject-2.0)
    add_library(gnome::gobject-2.0 STATIC IMPORTED)
    set_target_properties(gnome::gobject-2.0 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GLIB_SRC}/include/glib-2.0"
            IMPORTED_LOCATION "${GLIB_SRC}/lib/${GOBJECT_2_0_LIB}")
    target_link_libraries(gnome::gobject-2.0 INTERFACE gnu::libffi gnome::glib-2.0)
endif ()