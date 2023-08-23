file(GLOB_RECURSE MODULE_SRC *.cpp *.hpp)

add_library(wrtc STATIC ${MODULE_SRC})

set_property(TARGET wrtc PROPERTY CXX_STANDARD 20)

target_link_libraries(wrtc PUBLIC WebRTC::webrtc)

target_include_directories(wrtc PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${WEBRTC_INCLUDE}
)

setup_platform_libs(wrtc)