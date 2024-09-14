GetProperty("version.clang" CLANG_VERSION)
if (LINUX_x86_64 OR JUST_INSTALL_CLANG)
    set(CLANG_DIR ${deps_loc}/clang)
    set(CLANG_BIN_DIR ${CLANG_DIR}/bin)
    set(CLANG_UPDATE ${CLANG_DIR}/update.py)
    set(CLANG_API_DIR ${CLANG_DIR}/result.xml)

    if (NOT EXISTS ${CLANG_UPDATE} AND NOT EXISTS ${CLANG_BIN_DIR})
        GitFile(
            URL https://chromium.googlesource.com/chromium/src/tools/+/refs/heads/main/clang/scripts/update.py
            DIRECTORY ${CLANG_UPDATE}
        )
        set(URL_BASE "https://commondatastorage.googleapis.com/chromium-browser-clang/?delimiter=/&prefix=Linux_x64/")
        set(URL_TMP ${URL_BASE})
        set(BEST_GENERATION 0)
        set(CLANG_REVISION)
        set(CLANG_SUB_REVISION)

        while (1)
            file(DOWNLOAD ${URL_TMP} ${CLANG_API_DIR})
            file(READ ${CLANG_API_DIR} CLANG_API_CONTENT)
            PythonFindAllRegex(
                REGEX "<Key>Linux_x64/clang-(llvmorg-([0-9]+)-[\\w\-]+?)-([0-9]+)\.tgz</Key><Generation>([0-9]+)</Generation>"
                STRING "${CLANG_API_CONTENT}"
                VERSIONS
            )
            list(LENGTH VERSIONS num_results)
            math(EXPR VERSIONS_COUNT "${num_results} / 4 - 1")
            foreach (x RANGE 0 ${VERSIONS_COUNT})
                math(EXPR IDX "${x} * 4")
                set(CLANG_INFO)
                foreach (y RANGE 3)
                    math(EXPR NEW_IDX "${IDX} + ${y}")
                    list(GET VERSIONS ${NEW_IDX} TMP_VAR)
                    list(APPEND CLANG_INFO ${TMP_VAR})
                endforeach ()
                list(GET CLANG_INFO 0 FOUND_CLANG_REVISION)
                list(GET CLANG_INFO 1 FOUND_CLANG_VERSION)
                list(GET CLANG_INFO 2 FOUND_CLANG_SUB_REVISION)
                list(GET CLANG_INFO 3 GENERATION)
                if (${GENERATION} GREATER ${BEST_GENERATION} AND ${FOUND_CLANG_VERSION} STREQUAL ${CLANG_VERSION})
                    set(BEST_GENERATION ${GENERATION})
                    set(CLANG_REVISION ${FOUND_CLANG_REVISION})
                    set(CLANG_SUB_REVISION ${FOUND_CLANG_SUB_REVISION})
                endif ()
            endforeach ()
            PythonFindAllRegex(
                REGEX "<NextMarker>(.*?)</NextMarker>"
                STRING "${CLANG_API_CONTENT}"
                NEXT_MARKER
            )
            if (NEXT_MARKER)
                set(URL_TMP "${URL_BASE}&marker=${NEXT_MARKER}")
            else ()
                break()
            endif ()
        endwhile ()
        file(REMOVE ${CLANG_API_DIR})
        file(READ ${CLANG_UPDATE} FILE_CONTENT)
        PythonFindAllRegex(
            REGEX "CLANG_REVISION = '.*?'"
            STRING ${FILE_CONTENT}
            OLD_CLANG_REVISION
        )
        string(REPLACE ${OLD_CLANG_REVISION} "CLANG_REVISION = '${CLANG_REVISION}'" FILE_CONTENT ${FILE_CONTENT})
        PythonFindAllRegex(
            REGEX "CLANG_SUB_REVISION = [0-9]+"
            STRING ${FILE_CONTENT}
            OLD_CLANG_SUB_REVISION
        )
        string(REPLACE ${OLD_CLANG_SUB_REVISION} "CLANG_SUB_REVISION = ${CLANG_SUB_REVISION}" FILE_CONTENT ${FILE_CONTENT})
        PythonFindAllRegex(
            REGEX "RELEASE_VERSION = '[0-9]+'"
            STRING ${FILE_CONTENT}
            OLD_CLANG_RELEASE
        )
        string(REPLACE ${OLD_CLANG_RELEASE} "RELEASE_VERSION = '${CLANG_VERSION}'" FILE_CONTENT ${FILE_CONTENT})

        file(WRITE ${CLANG_UPDATE} ${FILE_CONTENT})
    endif ()

    if (NOT EXISTS ${CLANG_BIN_DIR})
        execute_process(
            COMMAND ${Python3_EXECUTABLE} ${CLANG_UPDATE} --output-dir ${CLANG_DIR}
        )
    endif ()
    if (NOT JUST_INSTALL_CLANG)
        set(CMAKE_C_COMPILER "${CLANG_BIN_DIR}/clang")
        set(CMAKE_CXX_COMPILER "${CLANG_BIN_DIR}/clang++")
    endif ()
elseif (LINUX_ARM64)
    set(CMAKE_C_COMPILER "clang-${CLANG_VERSION}")
    set(CMAKE_CXX_COMPILER "clang++-${CLANG_VERSION}")
endif ()
