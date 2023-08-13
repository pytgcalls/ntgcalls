file(GLOB_RECURSE MODULE_SRC *.cpp *.hpp)

pybind11_add_module(ntgcalls ${MODULE_SRC})
# add_library(ntgcalls STATIC ${MODULE_SRC})
#
set_property(TARGET ntgcalls PROPERTY CXX_STANDARD 20)
#
set_target_properties(ntgcalls PROPERTIES EXPORT_NAME ntgcalls)
#
# add_library(ntgcalls::ntgcalls ALIAS ntgcalls)

target_link_libraries(ntgcalls PRIVATE wrtc nlohmann_json::nlohmann_json)