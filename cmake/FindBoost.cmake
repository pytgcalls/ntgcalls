GetProperty("version.boost" BOOST_REVISION)
string(REPLACE "." "_" BOOST_REVISION_UNDERSCORE ${BOOST_REVISION})
set(BOOST_DIR ${deps_loc}/boost)
set(BOOST_WORKDIR ${BOOST_DIR}/src)
set(BOOST_ROOT ${BOOST_WORKDIR}/install)

# BUILD CONFIGS
set(B2_EXECUTABLE ./b2)
set(BOOST_LINK shared)
set(BOOST_VISIBILITY global)
set(BOOST_ARCH x86)
set(BOOST_TOOLSET clang)
if (WINDOWS_x86_64)
    set(B2_EXECUTABLE b2)
    set(BOOST_TARGET windows)
    set(BOOST_TOOLSET msvc)
    set(BOOST_CXX_FLAGS -D_ITERATOR_DEBUG_LEVEL=0)
    set(BOOST_LINK static)
elseif (LINUX_x86_64 OR LINUX_ARM64)
    set(BOOST_TARGET linux)
    set(BOOST_CXX_FLAGS
            -D_LIBCPP_ABI_NAMESPACE=Cr
            -D_LIBCPP_ABI_VERSION=2
            -D_LIBCPP_DISABLE_AVAILABILITY
            -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
            -nostdinc++
            -isystem${LIBCXX_INCLUDE}/include
            -fPIC
    )
    if (LINUX_ARM64)
        set(BOOST_ARCH arm)
    endif ()
elseif (MACOS_ARM64)
    set(BOOST_TARGET darwin)
    execute_process(COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE MAC_SYS_ROOT
        ERROR_QUIET
    )
    string(STRIP "${MAC_SYS_ROOT}" MAC_SYS_ROOT)
    set(BOOST_C_FLAGS
            --sysroot=${MAC_SYS_ROOT}
            -target aarch64-apple-darwin
    )
    set(BOOST_CXX_FLAGS
            -fPIC
            --sysroot=${MAC_SYS_ROOT}
            -std=gnu++17
            -target aarch64-apple-darwin
    )
    set(BOOST_VISIBILITY hidden)
    set(BOOST_ARCH arm)
else ()
    message(STATUS "[BOOST] ${CMAKE_SYSTEM_NAME} with ${CMAKE_HOST_SYSTEM_PROCESSOR} is not supported yet")
    return()
endif ()

if(NOT DEFINED LAST_BOOST_LIBS OR
        NOT "${LAST_BOOST_LIBS}" STREQUAL "${BOOST_LIBS}" OR
        NOT EXISTS ${BOOST_DIR} OR
        NOT EXISTS ${BOOST_WORKDIR} OR
        NOT EXISTS ${BOOST_ROOT})

    set(BOOST_DOWNLOAD_DIR ${BOOST_DIR}/download)
    DownloadProject(
        URL https://archives.boost.io/release/${BOOST_REVISION}/source/boost_${BOOST_REVISION_UNDERSCORE}.tar.gz
        DOWNLOAD_DIR ${BOOST_DOWNLOAD_DIR}
        SOURCE_DIR ${BOOST_WORKDIR}
    )
    if (NOT EXISTS ${BOOST_WORKDIR}/b2.exe OR NOT EXISTS ${BOOST_WORKDIR}/b2)
        if (WINDOWS)
            set(BOOTSTRAP_EXECUTABLE .\\bootstrap.bat)
        else ()
            set(BOOTSTRAP_EXECUTABLE ./bootstrap.sh)
        endif ()

        message(STATUS "[BOOST] Executing bootstrap...")
        execute_process(COMMAND ${BOOTSTRAP_EXECUTABLE}
            WORKING_DIRECTORY ${BOOST_WORKDIR}
            RESULT_VARIABLE rv
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(NOT rv EQUAL 0)
            file(REMOVE_RECURSE ${BOOST_WORKDIR})
            message(FATAL_ERROR "[BOOST] Error while executing bootstrap, cleaning up")
        endif ()
    endif ()

    if (EXISTS ${BOOST_ROOT})
        message(STATUS "[BOOST] Cleaning up old Boost build...")
        file(REMOVE_RECURSE ${BOOST_ROOT})
    endif ()

    if (NOT WINDOWS)
        file(WRITE ${BOOST_WORKDIR}/project-config.jam "using ${BOOST_TOOLSET} : : ${CMAKE_CXX_COMPILER} : ;")
    endif ()

    foreach(lib ${BOOST_LIBS})
        list(APPEND BOOST_LIBS_OPTIONS --with-${lib})
    endforeach()
    string (REPLACE ";" " " BOOST_C_FLAGS "${BOOST_C_FLAGS}")
    string (REPLACE ";" " " BOOST_CXX_FLAGS "${BOOST_CXX_FLAGS}")
    set(BUILD_COMMAND
        ${B2_EXECUTABLE}
        install
        -d+0
        --prefix=${BOOST_ROOT}
        ${BOOST_LIBS_OPTIONS}
        --layout=system
        --ignore-site-config
        variant=release
        cflags=${BOOST_C_FLAGS}
        cxxflags=${BOOST_CXX_FLAGS}
        toolset=${BOOST_TOOLSET}
        visibility=${BOOST_VISIBILITY}
        target-os=${BOOST_TARGET}
        address-model=64
        link=static
        runtime-link=${BOOST_LINK}
        threading=multi
        architecture=${BOOST_ARCH}
    )
    message(STATUS "[BOOST] Executing build process...")
    execute_process(COMMAND ${BUILD_COMMAND}
        WORKING_DIRECTORY ${BOOST_WORKDIR}
        RESULT_VARIABLE rv
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(NOT rv EQUAL 0)
        file(REMOVE_RECURSE ${BOOST_ROOT})
        string (REPLACE ";" " " BUILD_COMMAND "${BUILD_COMMAND}")
        message(FATAL_ERROR "[BOOST] Error while executing ${BUILD_COMMAND}, cleaning up")
    endif ()
    set(LAST_BOOST_LIBS ${BOOST_LIBS} CACHE STRING "Last boost libs" FORCE)
    message(STATUS "[BOOST] Build done")
endif ()
message(STATUS "boost v${BOOST_REVISION}")

set(Boost_USE_STATIC_LIBS ON)
if (WINDOWS)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

find_package(Boost REQUIRED COMPONENTS ${BOOST_LIBS})

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

add_compile_definitions(BOOST_ENABLED)
set(BOOST_ENABLED TRUE)