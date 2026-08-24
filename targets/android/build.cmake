add_subdirectory("${ROOT_DIR}/deps/oboe" "${CMAKE_BINARY_DIR}/deps/oboe")
setup_platform_flags(oboe OFF)

set(NTG_LIB_NAME ntgcalls-native)
add_library(${NTG_LIB_NAME} STATIC)
set_target_properties(${NTG_LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(${NTG_LIB_NAME} PRIVATE NTG_EXPORTS)
target_sources(${NTG_LIB_NAME} PRIVATE ${MODULE_SRC})
target_link_libraries(${NTG_LIB_NAME} PRIVATE oboe)
