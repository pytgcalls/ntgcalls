file(GLOB_RECURSE MODULE_SRC *.cpp)

set(BINDINGS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings)
set(NTG_LIB_NAME ntgcalls)

if(IS_PYTHON)
    list(REMOVE_ITEM MODULE_SRC "${BINDINGS_DIR}/ntgcalls.cpp")
    pybind11_add_module(${NTG_LIB_NAME} ${MODULE_SRC})
    add_compile_definitions(PYTHON_ENABLED)
else ()
    if (STATIC_BUILD OR ANDROID)
        set(NTG_LIB_NAME ntgcalls-native)
        add_library(${NTG_LIB_NAME} STATIC)
    else()
        add_library(${NTG_LIB_NAME} SHARED)
    endif ()

    set_target_properties(${NTG_LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    list(REMOVE_ITEM MODULE_SRC "${BINDINGS_DIR}/pythonapi.cpp")

    if (ANDROID_ABI)
        list(REMOVE_ITEM MODULE_SRC "${BINDINGS_DIR}/ntgcalls.cpp")
    else ()
        target_include_directories(${NTG_LIB_NAME} PRIVATE ../include)
    endif ()

    target_compile_definitions(${NTG_LIB_NAME} PRIVATE NTG_EXPORTS)
    target_sources(${NTG_LIB_NAME} PRIVATE ${MODULE_SRC})
endif ()

setup_platform_flags(${NTG_LIB_NAME} OFF)

target_include_directories(${NTG_LIB_NAME} PRIVATE include)
set_property(TARGET ${NTG_LIB_NAME} PROPERTY CXX_STANDARD 20 C_STANDARD 20)
target_link_libraries(${NTG_LIB_NAME} PRIVATE wrtc nlohmann_json::nlohmann_json)

if (NOT IS_PYTHON AND NOT ANDROID)
    set(ARCHIVE_TYPE)
    set(ARCHIVE_TYPE_NAME)
    if (STATIC_BUILD)
        set(ARCHIVE_TYPE ARCHIVE)
        set(ARCHIVE_TYPE_NAME "static")
    else ()
        if (WINDOWS)
            set(ARCHIVE_TYPE RUNTIME)
        else ()
            set(ARCHIVE_TYPE LIBRARY)
        endif ()
        set(ARCHIVE_TYPE_NAME "shared")
    endif ()
    set(ARCHIVE_DIR "${CMAKE_SOURCE_DIR}/${ARCHIVE_TYPE_NAME}-output")
    file(COPY ../include DESTINATION ${ARCHIVE_DIR})

    if (NOT STATIC_BUILD)
        set_target_properties(${NTG_LIB_NAME} PROPERTIES
            ${ARCHIVE_TYPE}_OUTPUT_DIRECTORY ${ARCHIVE_DIR}/lib
        )
        if (WINDOWS)
            set_target_properties(${NTG_LIB_NAME} PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_DIR}/lib
            )
        endif ()
    endif ()
endif ()

if (BOOST_ENABLED)
    target_link_libraries(${NTG_LIB_NAME} PRIVATE Boost::process)
endif ()

if (STATIC_BUILD OR ANDROID AND NOT IS_PYTHON)
    set(OUTPUT_ARCHIVE_DIR "${ARCHIVE_DIR}/lib")
    if (ANDROID)
        set(OUTPUT_ARCHIVE_DIR "${jni_loc}/ntgcalls/${ANDROID_ABI}")
    endif ()
    bundle_static_library(ntgcalls-native ntgcalls ${OUTPUT_ARCHIVE_DIR})
endif ()