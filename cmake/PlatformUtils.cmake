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

function(setup_platform_flags target_name import_libraries)
    if (WINDOWS)
        include(${cmake_dir}/Windows.cmake)
    elseif (ANDROID)
        include(${cmake_dir}/Android.cmake)
    elseif (LINUX)
        include(${cmake_dir}/Linux.cmake)
    elseif (MACOS)
        include(${cmake_dir}/macOS.cmake)
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

    if (EXISTS ${bundled_tgt_full_name})
        file(REMOVE ${bundled_tgt_full_name})
    endif ()

    if (LINUX OR ANDROID)
        file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "CREATE ${bundled_tgt_full_name}\n")

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
            INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
        )

        set(ar_tool ${CMAKE_AR})
        if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(ar_tool ${CMAKE_CXX_COMPILER_AR})
        endif()

        if (ANDROID)
            add_custom_command(
                COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
                COMMENT "Bundling ${bundled_tgt_name}"
                OUTPUT ${bundled_tgt_full_name}
                VERBATIM
            )
        else ()
            add_custom_command(
                COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
                COMMAND ${CMAKE_STRIP} --strip-unneeded ${bundled_tgt_full_name}
                COMMENT "Bundling and stripping ${bundled_tgt_name}"
                OUTPUT ${bundled_tgt_full_name}
                VERBATIM
            )
        endif ()
    elseif (WINDOWS)
        foreach(tgt IN LISTS static_libs)
            if (tgt MATCHES "^[A-Z]:")
                list(APPEND static_libs_full_names ${tgt})
            else ()
                list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
            endif ()
        endforeach()

        add_custom_command(
            COMMAND ${LIB_ASSEMBLER} /NOLOGO /IGNORE:4006 /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
            COMMAND ${CMAKE_STRIP} --strip-unneeded ${bundled_tgt_full_name}
            COMMENT "Bundling and stripping ${bundled_tgt_name}"
            OUTPUT ${bundled_tgt_full_name}
            VERBATIM
        )
    elseif (MACOS)
        find_program(lib_tool libtool)
        set(ar_tool ${lib_tool})

        file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "")

        foreach(tgt IN LISTS static_libs)
            if (tgt MATCHES "^/")
                file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "${tgt}\n")
            else ()
                file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "$<TARGET_FILE:${tgt}>\n")
            endif ()
        endforeach()

        file(GENERATE
            OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
            INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
        )

        add_custom_command(
            COMMAND ${ar_tool} -static -o "${bundled_tgt_full_name}" -filelist "${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar"
            COMMAND ${CMAKE_STRIP} -x "${bundled_tgt_full_name}"
            COMMENT "Bundling and stripping ${bundled_tgt_name}"
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