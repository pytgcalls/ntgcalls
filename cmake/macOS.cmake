enable_language(OBJCXX)
target_compile_options(${target_name} PRIVATE -fconstant-string-class=NSConstantString)
target_link_options(${target_name} PUBLIC -ObjC)
set_target_properties(${target_name} PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(${target_name} PUBLIC
    WEBRTC_POSIX
    WEBRTC_MAC
    NDEBUG
)

target_link_libraries(${target_name} PUBLIC
    "-framework AVFoundation"
    "-framework AudioToolbox"
    "-framework CoreAudio"
    "-framework QuartzCore"
    "-framework CoreMedia"
    "-framework VideoToolbox"
    "-framework AppKit"
    "-framework Metal"
    "-framework MetalKit"
    "-framework OpenGL"
    "-framework IOSurface"
    "-framework ScreenCaptureKit"
    "iconv"
)