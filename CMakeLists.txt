cmake_minimum_required(VERSION 3.23)

if (WIN32)
    add_compile_options(/wd4068)
endif ()

include(ExternalProject)

# https://chromiumdash.appspot.com/branches
set(WEBRTC_REVISION m120.6099.1.2)
set(BOOST_REVISION 1.84.0)
set(BOOST_LIBS filesystem)

if(DEFINED PY_VERSION_INFO)
    set(IS_PYTHON TRUE)
endif()

project(ntgcalls VERSION 1.0.4 LANGUAGES C CXX)
add_compile_definitions(NTG_VERSION="${CMAKE_PROJECT_VERSION}")

find_package(Threads REQUIRED)

get_filename_component(deps_loc "deps" REALPATH)

include(cmake/PlatformUtil.cmake)
include(cmake/DownloadProject.cmake)
include(cmake/GitUtils.cmake)
include(cmake/FindCURL.cmake)
include(cmake/FindWebRTC.cmake)
include(cmake/FindLibCXX.cmake)
include(cmake/FindBoost.cmake)

# pybind11
add_subdirectory(deps/pybind11)

# nlohmann/json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(deps/json EXCLUDE_FROM_ALL)

add_subdirectory(wrtc)
add_subdirectory(ntgcalls)
