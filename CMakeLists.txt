cmake_minimum_required(VERSION 3.27)

if (WIN32)
    add_compile_options(/wd4068)
endif ()

include(ExternalProject)

option(STATIC_BUILD "Build static libraries" ON)
option(USE_LIBCXX   "Use libc++" ON)

add_custom_target(clean_objects
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/cmake/CleanObjects.cmake"
    COMMENT "Removing all object (.o) files"
)

project(ntgcalls VERSION 2.0.0.3 LANGUAGES C CXX)
include(cmake/VersionUtil.cmake)

if (MACOS)
    enable_language(OBJCXX)
endif ()

find_package(Threads REQUIRED)

get_filename_component(deps_loc     "deps" REALPATH)
get_filename_component(android_loc  "android" REALPATH)
get_filename_component(jni_loc      "${android_loc}/app/src/main/jni" REALPATH)
get_filename_component(props_loc    "version.properties" REALPATH)

include(cmake/FindGLib.cmake)
include(cmake/FindX11.cmake)
include(cmake/FindMesa.cmake)
include(cmake/FindWebRTC.cmake)
include(cmake/FindLibCXX.cmake)
include(cmake/FindBoost.cmake)
include(cmake/FindOpenH264.cmake)
include(cmake/FindFFmpeg.cmake)

# pybind11
add_subdirectory(deps/pybind11)

# nlohmann/json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(deps/json EXCLUDE_FROM_ALL)

add_subdirectory(wrtc)
add_subdirectory(ntgcalls)
