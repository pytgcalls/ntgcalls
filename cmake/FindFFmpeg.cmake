set(FFMPEG_DIR ${deps_loc}/ffmpeg)

function(addFFmpeg target_name)
    if(WIN32 OR LINUX)
        find_package(PkgConfig REQUIRED)
        set(PKG_CONFIG_PATH "${FFMPEG_DIR}/lib/pkgconfig")
        pkg_check_modules(AVCODEC     REQUIRED NO_CMAKE_ENVIRONMENT_PATH IMPORTED_TARGET libavcodec)
        pkg_check_modules(AVFORMAT    REQUIRED NO_CMAKE_PATH IMPORTED_TARGET libavformat)
        pkg_check_modules(AVFILTER    REQUIRED IMPORTED_TARGET libavfilter)
        pkg_check_modules(AVDEVICE    REQUIRED IMPORTED_TARGET libavdevice)
        pkg_check_modules(AVUTIL      REQUIRED IMPORTED_TARGET libavutil)
        pkg_check_modules(SWRESAMPLE  REQUIRED IMPORTED_TARGET libswresample)
        pkg_check_modules(SWSCALE     REQUIRED IMPORTED_TARGET libswscale)

        add_library(FFmpeg INTERFACE IMPORTED GLOBAL)

        target_link_libraries(FFmpeg INTERFACE
                PkgConfig::AVCODEC
                PkgConfig::AVFORMAT
                PkgConfig::AVFILTER
                PkgConfig::AVDEVICE
                PkgConfig::AVUTIL
                PkgConfig::SWRESAMPLE
                PkgConfig::SWSCALE
                )
    else()
        message(FATAL_ERROR "Your OS is not supported yet")
    endif()
endfunction()

function(set_lib_dir varName libName)
    message("TEST ${libName}")

endfunction()