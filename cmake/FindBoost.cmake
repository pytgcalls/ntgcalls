set(BOOST_DIR ${deps_loc}/boost)
set(BOOST_SRC ${BOOST_DIR}/src)
set(BOOST_GIT https://github.com/pytgcalls/boost)
set(BOOST_ATOMIC_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}boost_atomic${CMAKE_STATIC_LIBRARY_SUFFIX})
set(BOOST_CONTEXT_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}boost_context${CMAKE_STATIC_LIBRARY_SUFFIX})
set(BOOST_DATE_TIME_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}boost_date_time${CMAKE_STATIC_LIBRARY_SUFFIX})
set(BOOST_SYSTEM_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}boost_system${CMAKE_STATIC_LIBRARY_SUFFIX})
set(BOOST_FILESYSTEM_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}boost_filesystem${CMAKE_STATIC_LIBRARY_SUFFIX})
set(BOOST_PROCESS_LIB ${CMAKE_STATIC_LIBRARY_PREFIX}boost_process${CMAKE_STATIC_LIBRARY_SUFFIX})

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
    message(STATUS "Boost is not supported on ${CMAKE_SYSTEM_NAME} with ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    return()
endif ()

GetProperty("version.boost" BOOST_VERSION)
message(STATUS "boost v${BOOST_VERSION}")

set(FILE_NAME boost.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})

DownloadProject(
    URL ${BOOST_GIT}/releases/download/v${BOOST_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${BOOST_DIR}/download
    SOURCE_DIR ${BOOST_SRC}
)

if (NOT TARGET Boost::atomic)
    add_library(Boost::atomic STATIC IMPORTED)
    set_target_properties(Boost::atomic PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOOST_SRC}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES CXX
        IMPORTED_LOCATION "${BOOST_SRC}/lib/${BOOST_ATOMIC_LIB}"
        INTERFACE_COMPILE_DEFINITIONS "BOOST_ATOMIC_NO_LIB"
    )
endif ()

if (NOT TARGET Boost::context)
    add_library(Boost::context STATIC IMPORTED)
    set_target_properties(Boost::context PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOOST_SRC}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES CXX
        IMPORTED_LOCATION "${BOOST_SRC}/lib/${BOOST_CONTEXT_LIB}"
        INTERFACE_COMPILE_DEFINITIONS "BOOST_CONTEXT_NO_LIB"
    )
endif ()

if (NOT TARGET Boost::date_time)
    add_library(Boost::date_time STATIC IMPORTED)
    set_target_properties(Boost::date_time PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOOST_SRC}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES CXX
        IMPORTED_LOCATION "${BOOST_SRC}/lib/${BOOST_DATE_TIME_LIB}"
        INTERFACE_COMPILE_DEFINITIONS "BOOST_DATE_TIME_NO_LIB"
    )
endif ()

if (NOT TARGET Boost::system)
    add_library(Boost::system STATIC IMPORTED)
    set_target_properties(Boost::system PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOOST_SRC}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES CXX
        IMPORTED_LOCATION "${BOOST_SRC}/lib/${BOOST_SYSTEM_LIB}"
        INTERFACE_COMPILE_DEFINITIONS "BOOST_SYSTEM_NO_LIB"
    )
endif ()

if (NOT TARGET Boost::filesystem)
    add_library(Boost::filesystem STATIC IMPORTED)
    set_target_properties(Boost::filesystem PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOOST_SRC}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES CXX
        IMPORTED_LOCATION "${BOOST_SRC}/lib/${BOOST_FILESYSTEM_LIB}"
        INTERFACE_COMPILE_DEFINITIONS "BOOST_FILESYSTEM_NO_LIB"
    )
    target_link_libraries(Boost::filesystem INTERFACE Boost::atomic Boost::system)
endif ()

if(NOT TARGET Boost::process)
    add_library(Boost::process STATIC IMPORTED)
    set_target_properties(Boost::process PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOOST_SRC}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES CXX
        IMPORTED_LOCATION "${BOOST_SRC}/lib/${BOOST_PROCESS_LIB}"
        INTERFACE_COMPILE_DEFINITIONS "BOOST_PROCESS_NO_LIB"
    )
    target_link_libraries(Boost::process INTERFACE Boost::atomic Boost::context Boost::date_time Boost::filesystem Boost::system)
endif ()

add_compile_definitions(BOOST_ENABLED)
set(BOOST_ENABLED TRUE)