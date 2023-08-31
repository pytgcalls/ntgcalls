cmake_minimum_required(VERSION 3.23)

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0091 NEW)

include(ExternalProject)

set(WEBRTC_REVISION m116.5845.6.1)
set(BOOST_REVISION 1.83.0)
set(BOOST_LIBS filesystem)

if(DEFINED PY_VERSION_INFO)
    set(IS_PYTHON TRUE)
endif()

project(ntgcalls VERSION 1.0.0 LANGUAGES C CXX)

find_package(Threads REQUIRED)

get_filename_component(deps_loc "deps" REALPATH)

include(cmake/PlatformUtil.cmake)
include(cmake/GitUtils.cmake)
include(cmake/FindCURL.cmake)
include(cmake/FindLibCXX.cmake)
include(cmake/FindWebRTC.cmake)
include(cmake/FindBoost.cmake)
include(cmake/SetupPlatformLibs.cmake)

# pybind11
add_subdirectory(deps/pybind11)

# nlohmann/json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(deps/json EXCLUDE_FROM_ALL)

add_subdirectory(wrtc)
add_subdirectory(ntgcalls)