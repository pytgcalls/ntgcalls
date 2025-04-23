set(X11_DIR ${deps_loc}/libx11)
set(X11_SRC ${X11_DIR}/src)
set(X11_GIT https://github.com/pytgcalls/libx11)
set(X11_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}X11${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XAU_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xau${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XCB_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}xcb${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XCOMPOSITE_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xcomposite${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XDAMAGE_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xdamage${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XEXT_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xext${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XFIXES_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xfixes${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XRENDER_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xrender${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XRANDR_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xrandr${CMAKE_STATIC_LIBRARY_SUFFIX})
set(XTST_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}Xtst${CMAKE_STATIC_LIBRARY_SUFFIX})

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

GetProperty("version.libX11" X11_VERSION)
message(STATUS "libX11 v${X11_VERSION}")

set(FILE_NAME libX11.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})
DownloadProject(
    URL ${X11_GIT}/releases/download/v${X11_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${X11_DIR}/download
    SOURCE_DIR ${X11_SRC}
)

if(NOT TARGET xorg::Xau)
    add_library(xorg::Xau STATIC IMPORTED)
    set_target_properties(xorg::Xau PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XAU_LIB}")
endif ()

if(NOT TARGET xorg::xcb)
    add_library(xorg::xcb STATIC IMPORTED)
    set_target_properties(xorg::xcb PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XCB_LIB}")
    target_link_libraries(xorg::xcb INTERFACE xorg::Xau)
endif ()

if(NOT TARGET xorg::X11)
    add_library(xorg::X11 STATIC IMPORTED)
    set_target_properties(xorg::X11 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${X11_SRC}/include"
            IMPORTED_LOCATION "${X11_SRC}/lib/${X11_LIB}")
    target_link_libraries(xorg::X11 INTERFACE xorg::xcb)
endif ()

if(NOT TARGET xorg::Xcomposite)
    add_library(xorg::Xcomposite STATIC IMPORTED)
    set_target_properties(xorg::Xcomposite PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XCOMPOSITE_LIB}")
endif ()

if(NOT TARGET xorg::Xdamage)
    add_library(xorg::Xdamage STATIC IMPORTED)
    set_target_properties(xorg::Xdamage PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XDAMAGE_LIB}")
endif ()

if(NOT TARGET xorg::Xext)
    add_library(xorg::Xext STATIC IMPORTED)
    set_target_properties(xorg::Xext PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XEXT_LIB}")
endif ()

if(NOT TARGET xorg::Xfixes)
    add_library(xorg::Xfixes STATIC IMPORTED)
    set_target_properties(xorg::Xfixes PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XFIXES_LIB}")
endif ()

if(NOT TARGET xorg::Xrender)
    add_library(xorg::Xrender STATIC IMPORTED)
    set_target_properties(xorg::Xrender PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XRENDER_LIB}")
    target_link_libraries(xorg::Xrender INTERFACE xorg::X11)
endif ()

if(NOT TARGET xorg::Xrandr)
    add_library(xorg::Xrandr STATIC IMPORTED)
    set_target_properties(xorg::Xrandr PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XRANDR_LIB}")
    target_link_libraries(xorg::Xrandr INTERFACE xorg::Xrender)
endif ()

if(NOT TARGET xorg::Xtst)
    add_library(xorg::Xtst STATIC IMPORTED)
    set_target_properties(xorg::Xtst PROPERTIES
            IMPORTED_LOCATION "${X11_SRC}/lib/${XTST_LIB}")
endif ()