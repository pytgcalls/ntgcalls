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
    target_link_libraries(${target_name} PUBLIC
        gnome::gio-2.0
        gnome::glib-2.0
        gnome::gobject-2.0
        mesa::gbm
        mesa::drm
        xorg::X11
        xorg::Xcomposite
        xorg::Xdamage
        xorg::Xext
        xorg::Xfixes
        xorg::Xrandr
        xorg::Xtst
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