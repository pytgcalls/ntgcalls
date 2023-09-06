file(GLOB_RECURSE MODULE_SRC *.cpp *.hpp)

set(BINDINGS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bindings)

if(IS_PYTHON)
    list(REMOVE_ITEM MODULE_SRC "${BINDINGS_DIR}/ntgcalls.cpp")
    pybind11_add_module(ntgcalls ${MODULE_SRC})
    target_compile_definitions(ntgcalls PRIVATE VERSION_INFO=${PY_VERSION_INFO})
else()
    list(REMOVE_ITEM MODULE_SRC "${BINDINGS_DIR}/pythonapi.cpp")
    add_library(ntgcalls SHARED ${MODULE_SRC})
    target_include_directories(ntgcalls PRIVATE ../include)
    target_compile_definitions(ntgcalls PRIVATE NTG_EXPORTS)
endif()

set_property(TARGET ntgcalls PROPERTY CXX_STANDARD 20)
target_link_libraries(ntgcalls PRIVATE wrtc nlohmann_json::nlohmann_json)
if(BOOST_ENABLED)
    target_link_libraries(ntgcalls PRIVATE Boost::filesystem)
endif ()

setup_platform_libs(ntgcalls)