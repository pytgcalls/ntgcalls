cmake_minimum_required(VERSION 3.27)

if (WIN32)
    add_compile_options(/wd4068)
endif ()

include(ExternalProject)

set(BOOST_LIBS filesystem)

option(STATIC_BUILD "Build static libraries" ON)
option(USE_LIBCXX   "Use libc++" ON)

if(DEFINED PY_VERSION_INFO)
    set(IS_PYTHON TRUE)
endif()

add_custom_target(clean_objects
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/cmake/CleanObjects.cmake"
    COMMENT "Removing all object (.o) files"
)

project(ntgcalls VERSION 1.3.0.14 LANGUAGES C CXX)
add_compile_definitions(NTG_VERSION="${CMAKE_PROJECT_VERSION}")

find_package(Threads REQUIRED)

get_filename_component(deps_loc     "deps" REALPATH)
get_filename_component(android_loc  "android" REALPATH)
get_filename_component(jni_loc      "${android_loc}/app/src/main/jni" REALPATH)
get_filename_component(props_loc    "version.properties" REALPATH)

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
