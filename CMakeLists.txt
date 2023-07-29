cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(ntgcalls VERSION 1.0 LANGUAGES C CXX)

set(NO_EXAMPLES ON)
set(NO_TESTS ON)
set(PYTHON_API ON)

add_subdirectory(deps/libdatachannel)
if (NOT TARGET nlohmann_json)
    add_subdirectory(deps/libdatachannel/deps/json ${CMAKE_BINARY_DIR}/deps/json)
endif()


file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.hpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/pythonapi.cpp")

add_library(ntgcalls SHARED ${SOURCES} ${HEADERS})

target_link_libraries(ntgcalls PRIVATE LibDataChannel::LibDataChannelStatic nlohmann_json::nlohmann_json)

set_property(TARGET ntgcalls PROPERTY CXX_STANDARD 17)

if(PYTHON_API)
    add_subdirectory(deps/pybind11)
    pybind11_add_module(ntgcalls-python src/pythonapi.cpp)
    set_property(TARGET ntgcalls-python PROPERTY CXX_STANDARD 17)
endif()