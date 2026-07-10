set(PYBIND11_FINDPYTHON "NEW")
add_subdirectory("${ROOT_DIR}/deps/pybind11" "${CMAKE_BINARY_DIR}/deps/pybind11")
pybind11_add_module(${NTG_LIB_NAME} ${MODULE_SRC} ${GEN_SOURCES})
target_include_directories(${NTG_LIB_NAME} PRIVATE ${GEN_INCLUDES})