set(LIBCXX_INCLUDE ${deps_loc}/libcxx)
set(LIBCXXABI_INCLUDE ${deps_loc}/libcxxabi)

if((LINUX OR ANDROID) AND USE_LIBCXX)
    GitClone(
        URL https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxx.git
        COMMIT ${LIBCXX_COMMIT}
        DIRECTORY ${LIBCXX_INCLUDE}
    )
    GitFile(
        URL https://chromium.googlesource.com/chromium/src/buildtools.git/+/${BUILDTOOLS_COMMIT}/third_party/libc++/__config_site
        DIRECTORY ${LIBCXX_INCLUDE}/include/__config_site
    )
    GitFile(
        URL https://chromium.googlesource.com/chromium/src/buildtools.git/+/${BUILDTOOLS_COMMIT}/third_party/libc++/__assertion_handler
        DIRECTORY ${LIBCXX_INCLUDE}/include/__assertion_handler
    )
    GitClone(
        URL https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxxabi.git
        COMMIT ${LIBCXX_ABI_COMMIT}
        DIRECTORY ${LIBCXXABI_INCLUDE}
    )
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>"
        "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:LIBCXX_INCLUDE_DIR>>:-isystem${LIBCXX_INCLUDE}/include>"
        "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:LIBCXXABI_INCLUDE_DIR>>:-isystem${LIBCXXABI_INCLUDE}/include>"
    )
endif ()