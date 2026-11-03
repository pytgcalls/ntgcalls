target_compile_definitions(${target_name} PRIVATE
    _WIN32_WINNT=0x0A00
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)
target_compile_definitions(${target_name} PUBLIC
    WEBRTC_WIN
    _LIBCPP_DISABLE_AVAILABILITY
    _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
    BOOST_NO_CXX98_FUNCTION_BASE
    RTC_ENABLE_H265
    NDEBUG
)
target_compile_options(${target_name} PRIVATE /utf-8 /bigobj)
set_target_properties(${target_name} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (import_libraries)
    target_link_libraries(${target_name} PUBLIC
        winmm.lib
        ws2_32.lib
        Strmiids.lib
        dmoguids.lib
        iphlpapi.lib
        msdmo.lib
        Secur32.lib
        wmcodecdspuuid.lib
        d3d11.lib
        dxgi.lib
        dwmapi.lib
        shcore.lib
        bcrypt.lib
        ntdll.lib
        userenv.lib
    )
endif ()
