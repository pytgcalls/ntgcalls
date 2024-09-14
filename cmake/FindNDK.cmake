if (ANDROID_ABI)
    set(NDK_DIR ${deps_loc}/ndk)
    set(NDK_SRC ${NDK_DIR}/src)

    GetProperty("version.ndk" NDK_VERSION)
    message(STATUS "android-ndk ${NDK_VERSION}")
    if (NOT EXISTS ${NDK_SRC})
        if (NOT LINUX)
            message(FATAL_ERROR "Only Linux is supported for Android build")
        endif ()
        DownloadProject(
            URL https://dl.google.com/android/repository/android-ndk-${NDK_VERSION}-linux.zip
            DOWNLOAD_DIR ${NDK_DIR}/download
            SOURCE_DIR ${NDK_SRC}
        )
    endif ()
    GetProperty("version.sdk_compile" ANDROID_NATIVE_API_LEVEL)
    set(ANDROID_NATIVE_API_LEVEL    ${ANDROID_NATIVE_API_LEVEL})
    set(ANDROID_PLATFORM            ${ANDROID_NATIVE_API_LEVEL})
    set(ANDROID_STL                 none)
    set(ANDROID_CPP_FEATURES        exceptions rtti)
    set(CMAKE_ANDROID_EXCEPTIONS    ON)
    set(ANDROID_NDK                 OFF)
    include(${NDK_SRC}/build/cmake/android.toolchain.cmake)
endif ()