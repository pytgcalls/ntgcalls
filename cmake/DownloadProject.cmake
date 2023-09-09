function(DownloadProject)
    cmake_parse_arguments(ARG "" "" "URL;DOWNLOAD_DIR;SOURCE_DIR;" ${ARGN})
    string(REGEX MATCH "[^/]*$" filename "${ARG_URL}")
    set(DOWNLOAD_OUTPUT_DIR ${ARG_DOWNLOAD_DIR}/${filename})
    if (NOT "${${ARG_URL}}" STREQUAL "${OS_FULL_NAME}")
        file(REMOVE_RECURSE ${DOWNLOAD_OUTPUT_DIR})
        file(REMOVE_RECURSE ${ARG_SOURCE_DIR})
    endif ()
    if (NOT EXISTS ${DOWNLOAD_OUTPUT_DIR})
        message(STATUS "Downloading...
        dst='${DOWNLOAD_OUTPUT_DIR}'")
        file(MAKE_DIRECTORY ${ARG_DOWNLOAD_DIR})
        message(STATUS "Using src='${ARG_URL}'")
        file(DOWNLOAD
                ${ARG_URL}
                ${DOWNLOAD_OUTPUT_DIR}
                SHOW_PROGRESS
                STATUS status
                LOG log
        )
        list(GET status 0 status_code)
        if(status_code EQUAL 0)
            message(STATUS "Downloading... done")
        else ()
            file(REMOVE_RECURSE ${ARG_DOWNLOAD_DIR})
            message(FATAL_ERROR "Error: downloading '${url}' failed
            status_code: ${status_code}
            status_string: ${status_string}
            log:
            --- LOG BEGIN ---
            ${log}
            --- LOG END ---"
            )
        endif ()
    endif ()
    if (NOT EXISTS ${ARG_SOURCE_DIR})
        message(STATUS "Extracting...
        src='${DOWNLOAD_OUTPUT_DIR}'
        dst='${ARG_SOURCE_DIR}'"
        )

        set(i 1234)
        while(EXISTS "${ARG_SOURCE_DIR}/../ex-project${i}")
            math(EXPR i "${i} + 1")
        endwhile()
        set(ut_dir "${ARG_SOURCE_DIR}/../ex-project${i}")
        file(MAKE_DIRECTORY "${ut_dir}")

        if(NOT EXISTS "${DOWNLOAD_OUTPUT_DIR}")
            message(FATAL_ERROR "File to extract does not exist: '${DOWNLOAD_OUTPUT_DIR}'")
        endif()
        file(MAKE_DIRECTORY ${ARG_SOURCE_DIR})
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${DOWNLOAD_OUTPUT_DIR}
            WORKING_DIRECTORY ${ut_dir}
            RESULT_VARIABLE rv
        )
        if(NOT rv EQUAL 0)
            message(STATUS "Extracting... [error clean up]")
            file(REMOVE_RECURSE "${ut_dir}")
            message(FATAL_ERROR "Extract of '${DOWNLOAD_OUTPUT_DIR}' failed")
        endif()

        message(STATUS "extracting... [analysis]")
        file(GLOB contents "${ut_dir}/*")
        list(REMOVE_ITEM contents "${ut_dir}/.DS_Store")
        list(LENGTH contents n)
        if(NOT n EQUAL 1 OR NOT IS_DIRECTORY "${contents}")
            set(contents "${ut_dir}")
        endif()

        message(STATUS "extracting... [rename]")
        file(REMOVE_RECURSE ${ARG_SOURCE_DIR})
        get_filename_component(contents ${contents} ABSOLUTE)
        file(RENAME ${contents} ${ARG_SOURCE_DIR})

        message(STATUS "extracting... [clean up]")
        file(REMOVE_RECURSE "${ut_dir}")

        set(${ARG_URL} "${OS_FULL_NAME}" CACHE STRING "Last project download" FORCE)

        message(STATUS "extracting... done")
    endif ()
endfunction()