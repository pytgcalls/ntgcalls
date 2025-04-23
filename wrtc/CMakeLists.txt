file(GLOB_RECURSE MODULE_SRC *.cpp)

add_library(wrtc STATIC)
set_property(TARGET wrtc PROPERTY CXX_STANDARD 20 C_STANDARD 20)
set_target_properties(wrtc PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(wrtc PUBLIC include)
target_sources(wrtc PRIVATE ${MODULE_SRC})

if (MACOS)
    file(GLOB_RECURSE MODULE_SRC *.mm)
    target_sources(wrtc PRIVATE ${MODULE_SRC})
endif ()

target_link_libraries(wrtc PUBLIC WebRTC::webrtc)
target_link_libraries(wrtc PRIVATE nlohmann_json::nlohmann_json)

setup_platform_flags(wrtc ON)

target_link_libraries(wrtc PUBLIC ffmpeg::avcodec ffmpeg::avformat ffmpeg::avutil ffmpeg::swresample)

if (NOT ANDROID)
    target_link_libraries(wrtc PRIVATE cisco::OpenH264)
endif ()