file(GLOB_RECURSE MODULE_SRC *.cpp)

set(BINDINGS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings)
set(TARGET_CODE_DIR "${ROOT_DIR}/targets/${BINDING}")
set(NTG_LIB_NAME ntgcalls)

include("${TARGET_CODE_DIR}/build.cmake")

if (MACOS)
    file(GLOB_RECURSE MODULE_MM_SRC *.mm)
    target_sources(${NTG_LIB_NAME} PRIVATE ${MODULE_MM_SRC})
endif ()

setup_platform_flags(${NTG_LIB_NAME} OFF)

target_include_directories(${NTG_LIB_NAME} PRIVATE include)
set_target_properties(${NTG_LIB_NAME} PROPERTIES CXX_STANDARD 20 C_STANDARD 20)
target_link_libraries(${NTG_LIB_NAME} PRIVATE wrtc)

if (BOOST_ENABLED)
    target_link_libraries(${NTG_LIB_NAME} PRIVATE Boost::process)
endif ()

include("${TARGET_CODE_DIR}/finalize.cmake" OPTIONAL)
