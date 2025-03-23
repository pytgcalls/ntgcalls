set(MESA_DIR ${deps_loc}/mesa)
set(MESA_SRC ${MESA_DIR}/src)
set(MESA_GIT https://github.com/pytgcalls/mesa)

if (LINUX_ARM64)
    set(PLATFORM linux)
    set(ARCHIVE_FORMAT .tar.gz)
    set(ARCH arm64)
elseif (LINUX_x86_64)
    set(PLATFORM linux)
    set(ARCHIVE_FORMAT .tar.gz)
    set(ARCH x86_64)
else ()
    return()
endif ()

GetProperty("version.mesa" MESA_VERSION)
message(STATUS "mesa v${MESA_VERSION}")

set(FILE_NAME mesa.${PLATFORM}-${ARCH}${ARCHIVE_FORMAT})
DownloadProject(
    URL ${MESA_GIT}/releases/download/v${MESA_VERSION}/${FILE_NAME}
    DOWNLOAD_DIR ${MESA_DIR}/download
    SOURCE_DIR ${MESA_SRC}
)

function(target_mesa_libraries target_name)
    set(interface_libs "")
    foreach (lib ${MESA_LIBS})
        set(link_option "-L${MESA_SRC}/lib -Wl,--push-state,-Bstatic,-l${lib},--pop-state")
        list(APPEND interface_libs ${link_option})
    endforeach ()
    target_link_libraries(${target_name} INTERFACE ${interface_libs})
endfunction()