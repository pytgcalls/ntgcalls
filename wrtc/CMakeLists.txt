file(GLOB_RECURSE MODULE_SRC *.cpp)

add_library(wrtc STATIC)
set_property(TARGET wrtc PROPERTY CXX_STANDARD 20 C_STANDARD 20)
set_target_properties(wrtc PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(wrtc PUBLIC include)
target_sources(wrtc PRIVATE ${MODULE_SRC})

target_link_libraries(wrtc PUBLIC WebRTC::webrtc)
target_link_libraries(wrtc PRIVATE nlohmann_json::nlohmann_json)

if (WINDOWS_x86_64)
    target_compile_definitions(wrtc PRIVATE
        _WIN32_WINNT=0x0A00
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    target_compile_definitions(wrtc PUBLIC
        WEBRTC_WIN
        RTC_ENABLE_H265
        _ITERATOR_DEBUG_LEVEL=0
        NDEBUG
    )
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
        d3d11.lib
        dxgi.lib
        dwmapi.lib
        shcore.lib
    )
elseif (ANDROID)
    target_compile_options(wrtc PUBLIC
        -fexperimental-relative-c++-abi-vtables
    )
    target_compile_definitions(wrtc PUBLIC
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
elseif (LINUX)
    target_compile_definitions(wrtc PUBLIC
        WEBRTC_POSIX
        WEBRTC_LINUX
        _LIBCPP_ABI_NAMESPACE=Cr
        _LIBCPP_ABI_VERSION=2
        _LIBCPP_DISABLE_AVAILABILITY
        BOOST_NO_CXX98_FUNCTION_BASE
        NDEBUG
        RTC_ENABLE_H265
        WEBRTC_USE_PIPEWIRE
        WEBRTC_USE_X11
    )
    target_link_static_libraries(${GLIB_SRC} wrtc
        ffi
        expat-full
        gio-2.0
        glib-2.0
        gobject-2.0
        gmodule-2.0
        gthread-2.0
        pcre2-8
        pcre2-posix
    )

    target_include_directories(wrtc
        INTERFACE
        ${GLIB_SRC}/include
    )

    target_link_static_libraries(${X11_SRC} wrtc
        X11
        X11-xcb
        xcb
        xcb-composite
        xcb-damage
        xcb-dbe
        xcb-dpms
        xcb-dri2
        xcb-dri3
        xcb-glx
        xcb-present
        xcb-randr
        xcb-record
        xcb-render
        xcb-res
        xcb-screensaver
        xcb-shape
        xcb-shm
        xcb-sync
        xcb-xf86dri
        xcb-xfixes
        xcb-xinerama
        xcb-xinput
        xcb-xkb
        xcb-xtest
        xcb-xv
        xcb-xvmc
        Xau
        Xcomposite
        Xdamage
        Xext
        Xfixes
        Xrandr
        Xrender
        Xtst
    )
    target_link_libraries(wrtc INTERFACE X11)

    target_link_static_libraries(${MESA_SRC} wrtc
        gbm
        drm
    )

    target_link_libraries(wrtc PRIVATE
        dl
        rt
        m
        z
        resolv
        -static-libgcc
        -static-libstdc++
        Threads::Threads
    )
elseif (MACOS)
    target_compile_definitions(wrtc PUBLIC
        WEBRTC_POSIX
        WEBRTC_MAC
        NDEBUG
    )
    enable_language(OBJCXX)
    target_compile_options(wrtc PRIVATE -fconstant-string-class=NSConstantString)
    target_link_options(wrtc PUBLIC -ObjC)
    set_target_properties(wrtc PROPERTIES CXX_VISIBILITY_PRESET hidden)
    target_link_libraries(wrtc PUBLIC
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
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} with ${OS_ARCH} is not supported yet")
endif ()

if (NOT ANDROID)
    target_link_libraries(wrtc PRIVATE cisco::OpenH264 ffmpeg::avcodec ffmpeg::avformat ffmpeg::avutil)
endif ()

if (ANDROID)
    set_target_properties(wrtc PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${jni_loc}/wrtc/${ANDROID_ABI}"
    )
endif ()