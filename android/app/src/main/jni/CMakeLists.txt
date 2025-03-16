cmake_minimum_required(VERSION 3.22.1)

project("ntgcalls" C CXX)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory("${DEPS_DIR}/json" "deps/json" EXCLUDE_FROM_ALL)

include(cmake/CompileUtils.cmake)
include(cmake/FindNTgCalls.cmake)
include(cmake/FindWRTC.cmake)
include(cmake/FindWebRTC.cmake)

find_package(Threads REQUIRED)

add_library(${CMAKE_PROJECT_NAME} SHARED
    ntgcalls.cpp
    jni_onload.cpp
    utils.cpp
    utils.hpp
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -B ${DEPS_DIR}/clang/bin")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>"
    "$<$<COMPILE_LANGUAGE:CXX>:-isystem${LIBCXX_INCLUDE_DIR}>"
)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE IS_ANDROID)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE NTgCalls::ntgcalls nlohmann_json::nlohmann_json android log OpenSLES EGL)
target_link_options(${CMAKE_PROJECT_NAME}
    PRIVATE
    ${WEBRTC_ANDROID_LDFLAGS}
)