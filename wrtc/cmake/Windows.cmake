target_compile_options(wrtc PRIVATE /utf-8 /bigobj)
set_target_properties(wrtc PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(wrtc PUBLIC
    winmm.lib
    ws2_32.lib
    Strmiids.lib
    dmoguids.lib
    iphlpapi.lib
    msdmo.lib
    Secur32.lib
    wmcodecdspuuid.lib
)
target_compile_definitions(wrtc PRIVATE
    _WIN32_WINNT=0x0A00
    WEBRTC_WIN
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)
target_compile_definitions(wrtc PUBLIC
    WEBRTC_WIN
    _ITERATOR_DEBUG_LEVEL=0
)