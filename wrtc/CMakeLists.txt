file(GLOB_RECURSE MODULE_SRC *.cpp *.hpp)

# add_library(wrtc STATIC ${MODULE_SRC})
add_executable(wrtc ${MODULE_SRC})

set_property(TARGET wrtc PROPERTY CXX_STANDARD 20)

target_link_libraries(wrtc PUBLIC WebRTC::webrtc)

if (WIN32)
  include(cmake/Windows.cmake)
elseif(LINUX)
  include(cmake/Linux.cmake)
endif()