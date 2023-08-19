target_compile_definitions(wrtc PUBLIC
    WEBRTC_POSIX
    WEBRTC_LINUX
)

set_target_properties(wrtc PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(wrtc PRIVATE
    X11
    dl
    rt
    Threads::Threads
)
