file(GLOB_RECURSE MODULE_SRC *.cpp *.hpp)

# add_library(wrtc STATIC ${MODULE_SRC})
add_executable(wrtc ${MODULE_SRC})

set_property(TARGET wrtc PROPERTY CXX_STANDARD 20)

target_link_libraries(wrtc PUBLIC WebRTC::webrtc)

if (WIN32)
  target_compile_options(wrtc PRIVATE /utf-8 /bigobj)
  set_target_properties(wrtc PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_link_libraries(wrtc PUBLIC
#    dbghelp.lib
#    delayimp.lib
#    dnsapi.lib
#    msimg32.lib
#    oleaut32.lib
#    psapi.lib
#    shell32.lib
#    shlwapi.lib
#    usp10.lib
#    version.lib
#    wininet.lib
    winmm.lib
    ws2_32.lib
#    amstrmid.lib
    Strmiids.lib
#    crypt32.lib
    dmoguids.lib
    iphlpapi.lib
    msdmo.lib
    Secur32.lib
    wmcodecdspuuid.lib
  )

  target_compile_definitions(wrtc PRIVATE
    _CONSOLE
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

endif()