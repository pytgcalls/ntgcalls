cmake_minimum_required(VERSION 3.27)

include(ExternalProject)

option(STATIC_BUILD "Build static libraries" ON)
option(USE_LIBCXX   "Use libc++" ON)
option(SCHEMA_ONLY  "Only generate the language-agnostic NTL schema, skip bindings and build" OFF)

add_custom_target(clean_objects
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/cmake/CleanObjects.cmake"
    COMMENT "Removing all object (.o) files"
)

project(ntgcalls VERSION 3.0.0.12 LANGUAGES C CXX)
include(cmake/VersionUtil.cmake)

if (MSVC)
    add_compile_options(/wd4068 /wd5030)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-unknown-attributes)
else ()
    add_compile_options(-Wno-attributes)
endif ()

if (MACOS)
    enable_language(OBJCXX)
endif ()

find_package(Threads REQUIRED)

get_filename_component(DEPS_DIR     "deps" REALPATH)
get_filename_component(ROOT_DIR     "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
get_filename_component(props_loc    "version.properties" REALPATH)

include(cmake/FindGLib.cmake)
include(cmake/FindX11.cmake)
include(cmake/FindMesa.cmake)
include(cmake/FindWebRTC.cmake)
include(cmake/FindLibCXX.cmake)
include(cmake/FindBoost.cmake)
include(cmake/FindOpenH264.cmake)
include(cmake/FindFFmpeg.cmake)
include(cmake/GenerateE2ETL.cmake)
include(cmake/codegen/GenerateSchema.cmake)
if(NOT SCHEMA_ONLY)
    include(cmake/codegen/GenerateBinding.cmake)

    add_subdirectory(wrtc)
    add_subdirectory(ntgcalls)
endif()
