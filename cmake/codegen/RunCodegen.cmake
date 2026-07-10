if (NOT DEFINED ROOT_DIR)
    get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." REALPATH)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/GenerateSchema.cmake)
