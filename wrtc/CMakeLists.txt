file(GLOB_RECURSE MODULE_SRC *.cpp)

add_library(wrtc STATIC)
set_property(TARGET wrtc PROPERTY CXX_STANDARD 20 C_STANDARD 20)

target_include_directories(wrtc PUBLIC include)
target_sources(wrtc PRIVATE ${MODULE_SRC})

target_link_libraries(wrtc PUBLIC WebRTC::webrtc)
target_link_libraries(wrtc PRIVATE nlohmann_json::nlohmann_json)

setup_platform_libs(wrtc)

if (ANDROID)
    set_target_properties(wrtc PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${android_loc}/library/src/main/jni/wrtc/${ANDROID_ABI}"
    )
endif ()