# static build: fuse ntgcalls-native and its dependencies into one archive
# (runs after the full link graph is known -- see ntgcalls/CMakeLists).
if (STATIC_BUILD)
    bundle_static_library(ntgcalls-native ntgcalls "${ARCHIVE_DIR}/lib")
endif ()