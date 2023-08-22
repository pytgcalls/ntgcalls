cmake_minimum_required(VERSION 3.23)

# Check platform
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(DEFINED PY_VERSION_INFO)
    set(IS_PYTHON TRUE)
endif()

project(ntgcalls VERSION 1.0.0 LANGUAGES C CXX)

find_package(Threads REQUIRED)

get_filename_component(deps_loc "deps" REALPATH)
set(WEBRTC_LIBRARY_DIR "${deps_loc}/libwebrtc/")
set(WEBRTC_INCLUDE_DIR "${WEBRTC_LIBRARY_DIR}include/")

include(cmake/FindWebRTC.cmake)
include(cmake/FindFFmpeg.cmake)
include(cmake/SetupPlatformLibs.cmake)

# pybind11
add_subdirectory(deps/pybind11)

# nlohmann/json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(deps/json EXCLUDE_FROM_ALL)

add_subdirectory(wrtc)
add_subdirectory(ntgcalls)