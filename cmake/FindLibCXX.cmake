if(NOT EXISTS ${deps_loc}/libcxx AND LINUX)
    GitClone(
        URL https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxx.git
        COMMIT main
        DIRECTORY ${deps_loc}/libcxx
    )
    GitFile(
        URL https://chromium.googlesource.com/chromium/src/buildtools.git/+/refs/heads/main/third_party/libc++/__config_site
        DIRECTORY ${deps_loc}/libcxx/include/__config_site
    )
endif ()
add_compile_options(
    "$<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:LIBCXX_INCLUDE_DIR>>:-isystem${deps_loc}/libcxx/include>"
)