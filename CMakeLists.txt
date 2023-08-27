cmake_minimum_required(VERSION 3.23)

include(ExternalProject)

set(WEBRTC_REVISION m116.5845.6.1)
# Check platform
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()
if (UNIX AND APPLE)
    set(MACOS TRUE)
endif ()

if(DEFINED PY_VERSION_INFO)
    set(IS_PYTHON TRUE)
endif()

project(ntgcalls VERSION 1.0.0 LANGUAGES C CXX)

find_package(Threads REQUIRED)

get_filename_component(deps_loc "deps" REALPATH)

include(cmake/GitUtils.cmake)
include(cmake/FindCURL.cmake)
include(cmake/FindLibCXX.cmake)
include(cmake/FindWebRTC.cmake)
include(cmake/SetupPlatformLibs.cmake)

# pybind11
add_subdirectory(deps/pybind11)

# nlohmann/json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(deps/json EXCLUDE_FROM_ALL)

add_subdirectory(wrtc)
add_subdirectory(ntgcalls)