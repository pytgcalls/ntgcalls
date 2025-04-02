set(cmake_dir ${CMAKE_SOURCE_DIR}/cmake)
if(WIN32)
    set(OS_NAME WINDOWS)
elseif (UNIX AND NOT APPLE)
    if (ANDROID_ABI)
        set(OS_NAME ANDROID)
    else ()
        set(OS_NAME LINUX)
    endif ()
elseif (UNIX AND APPLE)
    set(OS_NAME MACOS)
endif ()
set(${OS_NAME} ON)

add_compile_definitions(IS_${OS_NAME})

if (DEFINED CMAKE_OSX_ARCHITECTURES AND NOT
        CMAKE_OSX_ARCHITECTURES STREQUAL "")
    set(OS_ARCH ${CMAKE_OSX_ARCHITECTURES})
else ()
    set(OS_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif ()

if (OS_ARCH STREQUAL "AMD64" OR OS_ARCH STREQUAL "x86_64")
    set(${OS_NAME}_x86_64 ON)
elseif (OS_ARCH STREQUAL "aarch64" OR OS_ARCH STREQUAL "arm64")
    set(${OS_NAME}_ARM64 ON)
endif ()

set(OS_FULL_NAME ${CMAKE_SYSTEM_NAME};${OS_ARCH})

function(GetProperty key result)
    file(READ "${props_loc}" file_content)
    string(REPLACE "." "\." key "${key}")
    string(REGEX MATCH "${key}=([^\r\n]*)" _ ${file_content})
    set(${result} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()

function(target_link_static_libraries src target_name)
    set(static_flags "-L${src}/lib")
    set(static_group "")
    list(APPEND static_group "-Wl,--push-state")
    list(APPEND static_group "-Bstatic")
    list(APPEND static_group "-Wl,--start-group")
    foreach(lib IN LISTS ARGN)
        if(lib MATCHES "-full$")
            string(REGEX REPLACE "-full$" "" lib_no_full ${lib})
            list(APPEND static_group "-Wl,--whole-archive -l${lib_no_full} -Wl,--no-whole-archive")
        else()
            list(APPEND static_group "-Wl,-l${lib}")
        endif()
    endforeach()
    list(APPEND static_group "-Wl,--end-group")
    list(APPEND static_group "-Wl,--pop-state")
    target_link_libraries(${target_name} INTERFACE ${static_flags} ${static_group})
endfunction()

function(setup_platform_flags target_name import_libraries)
    if (WINDOWS_x86_64)
        target_compile_definitions(${target_name} PRIVATE
            _WIN32_WINNT=0x0A00
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            UNICODE
            _UNICODE
        )
        target_compile_definitions(${target_name} PUBLIC
            WEBRTC_WIN
            RTC_ENABLE_H265
            _ITERATOR_DEBUG_LEVEL=0
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
            )
        endif ()
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
        if (import_libraries)
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
        endif ()
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
        if (import_libraries)
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
        endif ()
    else()
        message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} with ${OS_ARCH} is not supported yet")
    endif ()
endfunction()

function(bundle_static_library tgt_name bundled_tgt_name bundle_output_dir)
    list(APPEND static_libs ${tgt_name})
    file(MAKE_DIRECTORY ${bundle_output_dir})

    function(recursively_collect_dependencies input_target)
        get_target_property(input_type ${input_target} TYPE)
        set(input_link_libraries INTERFACE_LINK_LIBRARIES)
        get_target_property(interface_dependencies ${input_target} INTERFACE_LINK_LIBRARIES)
        if (NOT interface_dependencies)
            get_target_property(interface_dependencies ${input_target} LINK_LIBRARIES)
        endif ()
        if (NOT interface_dependencies)
            get_target_property(interface_dependencies ${input_target} IMPORTED_LOCATION)
            if (NOT interface_dependencies)
                return()
            endif ()
            list(APPEND static_libs ${interface_dependencies})
            set(static_libs ${static_libs} PARENT_SCOPE)
            return()
        endif ()
        set(library_path "")
        set(group_opened false)
        foreach(dependency IN LISTS interface_dependencies)
            if (TARGET ${dependency} OR dependency MATCHES "^\\$<LINK_ONLY:(.+)>$")
                string(LENGTH "${CMAKE_MATCH_1}" length)
                if (${length} GREATER 0)
                    set(dependency ${CMAKE_MATCH_1})
                endif()
                if (TARGET ${dependency})
                    get_target_property(alias ${dependency} ALIASED_TARGET)
                    if (TARGET ${alias})
                        set(dependency ${alias})
                    endif()
                    get_target_property(_type ${dependency} TYPE)
                    if (${_type} STREQUAL "STATIC_LIBRARY")
                        list(APPEND static_libs ${dependency})
                    endif()
                    recursively_collect_dependencies(${dependency})
                endif ()
            elseif(dependency MATCHES "^-L(.+)$")
                set(library_path "${CMAKE_MATCH_1}")
            elseif (dependency MATCHES "^-Wl,--start-group")
                set(group_opened true)
            elseif (dependency MATCHES "^-Wl,--end-group")
                set(group_opened false)
            elseif (dependency MATCHES "-l([^ ]+)" AND group_opened)
                list(APPEND static_libs "${library_path}/${CMAKE_STATIC_LIBRARY_PREFIX}${CMAKE_MATCH_1}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif ()
        endforeach ()
        set(static_libs ${static_libs} PARENT_SCOPE)
    endfunction()

    recursively_collect_dependencies(${tgt_name})
    list(REMOVE_DUPLICATES static_libs)

    set(bundled_tgt_full_name ${bundle_output_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

    if (CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
        file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
                "CREATE ${bundled_tgt_full_name}\n" )
        foreach(tgt IN LISTS static_libs)
            if (tgt MATCHES "^/")
                file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
                        "ADDLIB ${tgt}\n")
            else ()
                file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
                        "ADDLIB $<TARGET_FILE:${tgt}>\n")
            endif ()
        endforeach()

        file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
        file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "END\n")

        file(GENERATE
            OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
            INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in)

        set(ar_tool ${CMAKE_AR})
        if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(ar_tool ${CMAKE_CXX_COMPILER_AR})
        endif()

        add_custom_command(
            COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
            COMMENT "Bundling ${bundled_tgt_name}"
            OUTPUT ${bundled_tgt_full_name}
            VERBATIM
        )
    elseif (MSVC)
        find_program(lib_tool lib)

        foreach(tgt IN LISTS static_libs)
            if (tgt MATCHES "^/")
                list(APPEND static_libs_full_names ${tgt})
            else ()
                list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
            endif ()
        endforeach()

        add_custom_command(
            COMMAND ${lib_tool} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
            COMMENT "Bundling ${bundled_tgt_name}"
            OUTPUT ${bundled_tgt_full_name}
            VERBATIM
        )
    else ()
        message(FATAL_ERROR "Unsupported compiler for static library bundling")
    endif ()

    add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
    add_dependencies(bundling_target ${tgt_name})

    add_library(${bundled_tgt_name} STATIC IMPORTED)
    set_target_properties(${bundled_tgt_name}
        PROPERTIES
        IMPORTED_LOCATION ${bundled_tgt_full_name}
        INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>
    )
    add_dependencies(${bundled_tgt_name} bundling_target)
endfunction()