# C ABI target: a native shared (or static) library exposing the generated C
# entry points (ntgcalls.h / ntgcalls_c.cpp), consumed by the go/rust/desktop FFI.
if (STATIC_BUILD)
    set(NTG_LIB_NAME ntgcalls-native)
    add_library(${NTG_LIB_NAME} STATIC)
else ()
    add_library(${NTG_LIB_NAME} SHARED)
endif ()

set_target_properties(${NTG_LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(${NTG_LIB_NAME} PRIVATE NTG_EXPORTS)
target_include_directories(${NTG_LIB_NAME} PRIVATE ${TARGET_CODE_DIR} ${GEN_INCLUDES})
target_sources(${NTG_LIB_NAME} PRIVATE ${MODULE_SRC} ${GEN_SOURCES})

# packaged artefacts (public headers + library) under <mode>-output/
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
file(COPY "${TARGET_CODE_DIR}/ntgcalls.h" DESTINATION "${ARCHIVE_DIR}/include")

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
