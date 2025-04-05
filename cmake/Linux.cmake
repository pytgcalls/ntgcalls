target_compile_definitions(${target_name} PUBLIC
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

if (import_libraries)
    target_link_static_libraries(${GLIB_SRC} ${target_name}
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

    target_include_directories(${target_name}
        INTERFACE
        ${GLIB_SRC}/include
    )

    target_link_static_libraries(${X11_SRC} ${target_name}
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
    target_link_libraries(${target_name} INTERFACE X11)

    target_link_static_libraries(${MESA_SRC} ${target_name}
        gbm
        drm-full
    )

    target_link_libraries(${target_name} PRIVATE
        dl
        rt
        m
        z
        resolv
        Threads::Threads
    )
endif ()