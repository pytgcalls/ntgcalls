file(GLOB_RECURSE MODULE_SRC *.cpp *.hpp)

if(UNIX AND NOT APPLE)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

# add_library(wrtc STATIC ${MODULE_SRC})
add_executable(wrtc ${MODULE_SRC})

set_property(TARGET wrtc PROPERTY CXX_STANDARD 20)

target_include_directories(wrtc PUBLIC
    ${libwebrtc_source_dir}
    ${libwebrtc_source_dir}/webrtc
    ${libwebrtc_source_dir}/webrtc/api
    ${libwebrtc_source_dir}/webrtc/third_party/abseil-cpp
    ${libwebrtc_source_dir}/webrtc/third_party/libyuv/include
)

target_include_directories(wrtc PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(wrtc PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    libwebrtc
)

target_compile_definitions(wrtc PUBLIC
    -DUSE_BUILTIN_SW_CODECS
)

if(WIN32)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /GR-")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /GR- -D_HAS_ITERATOR_DEBUGGING=0")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /FORCE:UNRESOLVED")

  target_link_libraries(wrtc PUBLIC
      dmoguids.lib
      msdmo.lib
      secur32.lib
      winmm.lib
      wmcodecdspuuid.lib
      ws2_32.lib
  )

  target_compile_definitions(wrtc PUBLIC
      -D_WINSOCKAPI_
      -DNOGDI
      -DNOMINMAX
      -DWEBRTC_WIN
  )
else()
  target_include_directories(wrtc PUBLIC
      ${libc++_include_dir}
      ${libc++abi_include_dir}
  )

  # NOTE(mroberts): On Linux and macOS, we statically link the same libc++ and
  # libc++abi libraries that libwebrtc builds against.
  target_link_libraries(wrtc PUBLIC
      -lc
      -lm
      libc++
  )

  if(NOT APPLE)
    target_link_libraries(wrtc PUBLIC
        libc++abi
    )
  endif()

  target_compile_options(wrtc PUBLIC
      -fvisibility=hidden
      -nostdinc++
      -nodefaultlibs
      -Wall
      -Wextra
      -Wno-missing-field-initializers
      -Wno-uninitialized
      -Wno-unused-local-typedefs
      -Wno-unused-variable
      -Wpedantic
  )

  target_compile_definitions(wrtc PUBLIC
      -D_LIBCPP_ABI_UNSTABLE
      -DWEBRTC_POSIX=1
  )

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(wrtc PUBLIC
        -g
#        -fsanitize=address
#        -fno-omit-frame-pointer
    )

    target_compile_definitions(wrtc PUBLIC
        -DDEBUG
    )
  endif()

  if(APPLE)
    target_link_libraries(wrtc PUBLIC
        "-framework AppKit"
        "-framework AVFoundation"
    )

    if ("$ENV{TARGET_ARCH}" STREQUAL "arm64")
      target_compile_options(wrtc PUBLIC
          -arch arm64
      )
      target_link_options(wrtc PUBLIC
          -arch arm64
      )
    endif()

    target_compile_options(wrtc PUBLIC
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-documentation
        -Wno-exit-time-destructors
        -Wno-float-conversion
        -Wno-global-constructors
        -Wno-padded
        -Wno-shadow
        -Wno-shadow-field-in-constructor
        -Wno-shorten-64-to-32
        -Wno-sign-conversion
        -Wno-thread-safety-negative
        -Wno-unused-template
        -Wno-weak-vtables
    )

    target_compile_definitions(wrtc PUBLIC
        -D_LIBCPP_ABI_UNSTABLE
        -DWEBRTC_MAC
        -DWEBRTC_IOS
    )
  else()
    target_compile_definitions(wrtc PUBLIC
        -DWEBRTC_LINUX
    )

    target_compile_options(wrtc PUBLIC
        -fpermissive
    )

    if ("$ENV{TARGET_ARCH}" STREQUAL "arm" OR "$ENV{TARGET_ARCH}" STREQUAL "arm64")
      set(CMAKE_SYSTEM_NAME Linux)
      set(CMAKE_SYSTEM_PROCESSOR "$ENV{TARGET_ARCH}")
      set(tools $ENV{ARM_TOOLS_PATH})

      set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
      set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

      if ("$ENV{TARGET_ARCH}" STREQUAL "arm")
        set(CMAKE_SYSROOT ${libwebrtc_source_dir}/src/build/linux/debian_sid_arm-sysroot)
        target_compile_options(wrtc PUBLIC
            -march=armv7-a
            -mfloat-abi=hard
            -mtune=generic-armv7-a
            -mfpu=neon
        )
        set(CMAKE_C_COMPILER ${tools}/bin/arm-linux-gnueabihf-gcc)
        set(CMAKE_CXX_COMPILER ${tools}/bin/arm-linux-gnueabihf-g++)
      else()
        set(CMAKE_SYSROOT ${libwebrtc_source_dir}/src/build/linux/debian_sid_arm64-sysroot)
        set(CMAKE_C_COMPILER ${tools}/bin/aarch64-linux-gnu-gcc)
        set(CMAKE_CXX_COMPILER ${tools}/bin/aarch64-linux-gnu-g++)
      endif()
    else()
      target_compile_options(wrtc PUBLIC
          -B${libwebrtc_source_dir}/src/third_party/binutils/Linux_x64/Release/bin
          --sysroot=${libwebrtc_source_dir}/src/build/linux/debian_sid_amd64-sysroot
      )
    endif()
  endif()
endif()
