cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(tgcalls VERSION 1.0 LANGUAGES C CXX)

set(NO_EXAMPLES ON)
set(NO_TESTS ON)
set(PYTHON_API ON)

add_subdirectory(deps/libdatachannel)
if (NOT TARGET nlohmann_json)
    add_subdirectory(deps/libdatachannel/deps/json ${CMAKE_BINARY_DIR}/deps/json)
endif()


file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.hpp")
list(REMOVE_ITEM SOURCES "src/pythonapi.cpp")

add_library(tgcalls SHARED ${SOURCES} ${HEADERS} src/SdpBuilder.cpp src/SdpBuilder.hpp src/stream.cpp src/stream.hpp)

target_link_libraries(tgcalls PRIVATE LibDataChannel::LibDataChannelStatic nlohmann_json::nlohmann_json)

set_property(TARGET tgcalls PROPERTY CXX_STANDARD 17)

if(PYTHON_API)
    add_subdirectory(deps/pybind11)
    pybind11_add_module(tgcalls-python src/pythonapi.cpp)
    set_property(TARGET tgcalls-python PROPERTY CXX_STANDARD 17)
endif()


#if(MSVC OR MINGW)
#    add_custom_command(TARGET tgcalls POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            "$<TARGET_FILE_DIR:datachannel>/libdatachannel.dll"
#            $<TARGET_FILE_DIR:tgcalls>
#            )
#endif()