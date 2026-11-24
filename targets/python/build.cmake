set(PYBIND11_FINDPYTHON "NEW")
if (WIN32 AND DEFINED Python_EXECUTABLE)
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import sys, sysconfig; t = 't' if sysconfig.get_config_var('Py_GIL_DISABLED') else ''; print(f\"{sys.base_prefix}/libs/python{sysconfig.get_config_var('py_version_nodot')}{t}.lib;{sysconfig.get_paths()['include']};{t}\", end='')"
        OUTPUT_VARIABLE PYTHON_INFO
    )
    list(GET PYTHON_INFO 0 Python_LIBRARY)
    list(GET PYTHON_INFO 1 Python_INCLUDE_DIR)
    list(GET PYTHON_INFO 2 PYTHON_ABI)
    file(TO_CMAKE_PATH "${Python_LIBRARY}" Python_LIBRARY)
    file(TO_CMAKE_PATH "${Python_INCLUDE_DIR}" Python_INCLUDE_DIR)
endif ()
add_subdirectory("${ROOT_DIR}/deps/pybind11" "${CMAKE_BINARY_DIR}/deps/pybind11")
pybind11_add_module(${NTG_LIB_NAME} ${MODULE_SRC} ${GEN_SOURCES})
target_include_directories(${NTG_LIB_NAME} PRIVATE ${GEN_INCLUDES})
if (PYTHON_ABI STREQUAL "t")
    target_compile_definitions(${NTG_LIB_NAME} PRIVATE Py_GIL_DISABLED=1)
endif ()
