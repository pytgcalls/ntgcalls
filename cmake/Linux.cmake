target_compile_definitions(${target_name} PUBLIC
    WEBRTC_POSIX
    WEBRTC_LINUX
    _LIBCPP_ABI_NAMESPACE=Cr
    _LIBCPP_ABI_VERSION=2
    _LIBCPP_DISABLE_AVAILABILITY
)

set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${target_name} PRIVATE
    X11
    dl
    rt
    Threads::Threads
)
