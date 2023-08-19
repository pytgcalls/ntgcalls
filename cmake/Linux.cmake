target_compile_definitions(${target_name} PUBLIC
    WEBRTC_POSIX
    WEBRTC_LINUX
)

set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${target_name} PRIVATE
    X11
    dl
    rt
    Threads::Threads
)
