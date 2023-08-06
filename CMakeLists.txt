cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(ntgcalls VERSION 1.0.0 LANGUAGES C CXX)

set(NO_EXAMPLES ON)
set(NO_TESTS ON)
set(SOURCES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(deps/libdatachannel)
if (NOT TARGET nlohmann_json)
    add_subdirectory(deps/libdatachannel/deps/json ${CMAKE_BINARY_DIR}/deps/json)
endif()


file(GLOB_RECURSE SOURCES "${SOURCES_ROOT}/*.cpp")
file(GLOB_RECURSE HEADERS "${SOURCES_ROOT}/*.hpp")
list(REMOVE_ITEM SOURCES "${SOURCES_ROOT}/pythonapi.cpp")

if(DEFINED PY_VERSION_INFO)
    add_subdirectory(deps/pybind11)
    pybind11_add_module(ntgcalls src/pythonapi.cpp ${SOURCES} ${HEADERS})
    target_compile_definitions(ntgcalls PRIVATE VERSION_INFO=${PY_VERSION_INFO})
else()
    add_library(ntgcalls SHARED ${SOURCES} ${HEADERS})
endif()

target_link_libraries(ntgcalls PRIVATE LibDataChannel::LibDataChannelStatic nlohmann_json::nlohmann_json)

set_property(TARGET ntgcalls PROPERTY CXX_STANDARD 20)