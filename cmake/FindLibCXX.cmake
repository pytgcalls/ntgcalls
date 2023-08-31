if(LINUX)
    set(LIBCXX_INCLUDE ${deps_loc}/libcxx)
    set(LIBCXXABI_INCLUDE ${deps_loc}/libcxxabi)
    if (NOT EXISTS ${LIBCXX_INCLUDE})
        GitClone(
            URL https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxx.git
            COMMIT main
            DIRECTORY ${LIBCXX_INCLUDE}
        )
        GitFile(
            URL https://chromium.googlesource.com/chromium/src/buildtools.git/+/refs/heads/main/third_party/libc++/__config_site
            DIRECTORY ${LIBCXX_INCLUDE}/include/__config_site
        )
    endif ()
    if (NOT EXISTS ${LIBCXXABI_INCLUDE})
        GitClone(
            URL https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxxabi.git
            COMMIT main
            DIRECTORY ${LIBCXXABI_INCLUDE}
        )
    endif ()
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>"
        "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:LIBCXX_INCLUDE_DIR>>:-isystem${LIBCXX_INCLUDE}/include>"
        "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:LIBCXXABI_INCLUDE_DIR>>:-isystem${LIBCXXABI_INCLUDE}/include>"
    )
endif ()