if (NOT DEFINED CMAKE_JS_INC)
    message(FATAL_ERROR
            "Node headers not found. Build the node target through cmake-js "
            "(setup.py build_lib --target=node), not raw cmake.")
endif ()

execute_process(
    COMMAND node -p "require('node-addon-api').include_dir"
    WORKING_DIRECTORY ${TARGET_CODE_DIR}
    OUTPUT_VARIABLE NAPI_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE NAPI_LOOKUP
)
if (NOT NAPI_LOOKUP EQUAL 0)
    message(FATAL_ERROR
            "node-addon-api not found. Run npm install in ${TARGET_CODE_DIR}.")
endif ()
get_filename_component(NAPI_INCLUDE_DIR "${NAPI_INCLUDE_DIR}" ABSOLUTE BASE_DIR "${TARGET_CODE_DIR}")

add_library(${NTG_LIB_NAME} SHARED ${MODULE_SRC} ${GEN_SOURCES} ${CMAKE_JS_SRC})
set_target_properties(${NTG_LIB_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(${NTG_LIB_NAME} PRIVATE
    ${CMAKE_JS_INC}
    ${NAPI_INCLUDE_DIR}
    ${GEN_INCLUDES}
)
target_link_libraries(${NTG_LIB_NAME} PRIVATE ${CMAKE_JS_LIB})

target_compile_definitions(${NTG_LIB_NAME} PRIVATE
    NAPI_VERSION=8
    NAPI_CPP_EXCEPTIONS
)

if (WINDOWS AND MSVC)
    target_link_options(${NTG_LIB_NAME} PRIVATE /DELAYLOAD:NODE.EXE)
endif ()

set(NODE_PKG_DIR "${TARGET_CODE_DIR}/build/Release")
set_target_properties(${NTG_LIB_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${NODE_PKG_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${NODE_PKG_DIR}"
)