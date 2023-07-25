cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(tgcalls VERSION 1.0 LANGUAGES CXX)

set(NO_EXAMPLES ON)
set(NO_TESTS ON)

add_subdirectory(deps/libdatachannel)
add_subdirectory(deps/libdatachannel/deps/json)

add_executable(tgcalls src/ntgcalls.cpp src/ntgcalls.h)

set_property(TARGET tgcalls PROPERTY CXX_STANDARD 20)

target_link_libraries(tgcalls PRIVATE LibDataChannel::LibDataChannel nlohmann_json::nlohmann_json)