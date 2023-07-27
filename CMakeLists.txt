cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(tgcalls VERSION 1.0 LANGUAGES CXX)

set(NO_EXAMPLES ON)
set(NO_TESTS ON)

add_subdirectory(deps/libdatachannel)
add_subdirectory(deps/libdatachannel/deps/json)

# add_executable(tgcalls src/tgcalls.hpp src/tgcalls.cpp src/main.cpp src/utils.cpp src/utils.hpp)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.hpp")

add_library(tgcalls SHARED ${SOURCES} ${HEADERS})

target_link_libraries(tgcalls PRIVATE LibDataChannel::LibDataChannel nlohmann_json::nlohmann_json)

set_property(TARGET tgcalls PROPERTY CXX_STANDARD 17)

if(MSVC OR MINGW)
    add_custom_command(TARGET tgcalls POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:datachannel>/libdatachannel.dll"
            $<TARGET_FILE_DIR:tgcalls>
            )
endif()