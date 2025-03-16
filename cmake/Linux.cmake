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

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GOBJECT REQUIRED IMPORTED_TARGET gobject-2.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

target_include_directories(${target_name}
    INTERFACE
    ${GIO_INCLUDE_DIRS}
    ${GOBJECT_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
)

target_link_libraries(${target_name}
    INTERFACE
    gio-2.0
    gobject-2.0
    glib-2.0
)

set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${target_name} PRIVATE
    X11
    gbm
    drm
    Xcomposite
    Xdamage
    Xext
    Xfixes
    Xrandr
    Xrender
    Xtst
    dl
    rt
    m
    Threads::Threads
)
