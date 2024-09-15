cmake_minimum_required(VERSION 3.22.1)

project("ntgcalls" C CXX)

include(cmake/AndroidPlatformUtil.cmake)
include(cmake/FindNTgCalls.cmake)
include(cmake/FindWRTC.cmake)
include(cmake/FindWebRTC.cmake)


add_library(${CMAKE_PROJECT_NAME} SHARED
    ntgcalls.cpp
    utils.cpp
    utils.hpp
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -B ${DEPS_DIR}/clang/bin")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>"
    "$<$<COMPILE_LANGUAGE:CXX>:-isystem${LIBCXX_INCLUDE_DIR}>"
)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE IS_ANDROID)
setup_platform_libs(${CMAKE_PROJECT_NAME})

target_link_libraries(${CMAKE_PROJECT_NAME} WebRTC::webrtc Wrtc::wrtc NTgCalls::ntgcalls android log)
target_link_options(${CMAKE_PROJECT_NAME}
    PRIVATE
    ${WEBRTC_ANDROID_LDFLAGS}
)