file(GLOB_RECURSE MODULE_SRC *.cpp *.hpp)

if(IS_PYTHON)
    pybind11_add_module(ntgcalls ${MODULE_SRC})
    target_compile_definitions(ntgcalls PRIVATE VERSION_INFO=${PY_VERSION_INFO})
else()
    list(REMOVE_ITEM MODULE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/pythonapi.cpp")
    add_library(ntgcalls SHARED ${MODULE_SRC})
endif()

set_property(TARGET ntgcalls PROPERTY CXX_STANDARD 20)

target_link_libraries(ntgcalls PRIVATE wrtc nlohmann_json::nlohmann_json)

setup_platform_libs(ntgcalls)