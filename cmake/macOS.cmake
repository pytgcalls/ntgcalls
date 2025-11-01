target_compile_definitions(${target_name} PUBLIC
    WEBRTC_POSIX
    WEBRTC_MAC
    _LIBCPP_ABI_NAMESPACE=Cr
    _LIBCPP_ABI_VERSION=2
    _LIBCPP_DISABLE_AVAILABILITY
    _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCPP_ENABLE_NODISCARD
    _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
    NDEBUG
)
target_compile_options(${target_name} PRIVATE -fconstant-string-class=NSConstantString)
target_link_options(${target_name} PUBLIC -ObjC)
set_target_properties(${target_name} PROPERTIES CXX_VISIBILITY_PRESET hidden)

if (import_libraries)
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
        "z"
        "bz2"
    )
endif ()