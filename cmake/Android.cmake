if (ANDROID_ABI STREQUAL "arm64-v8a")
    target_compile_options(${target_name} PUBLIC
        -fexperimental-relative-c++-abi-vtables
    )
endif ()
target_link_options(${target_name} PRIVATE
    -Wl,-z,max-page-size=16384
)
target_compile_definitions(${target_name} PUBLIC
    WEBRTC_POSIX
    WEBRTC_LINUX
    WEBRTC_ANDROID
    _LIBCPP_ABI_NAMESPACE=Cr
    _LIBCPP_ABI_VERSION=2
    _LIBCPP_DISABLE_AVAILABILITY
    _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
    _LIBCPP_ENABLE_NODISCARD
    _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
    BOOST_NO_CXX98_FUNCTION_BASE
)