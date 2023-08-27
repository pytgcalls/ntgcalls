function(GitClone)
    cmake_parse_arguments(ARG "" "" "URL;COMMIT;DIRECTORY" ${ARGN})
    string(REPLACE "https" "" GIT_CACHE ${ARG_URL})
    string(REPLACE "http" "" GIT_CACHE ${GIT_CACHE})
    string(REPLACE "//" "" GIT_CACHE ${GIT_CACHE})
    string(REPLACE ":" "" GIT_CACHE ${GIT_CACHE})
    string(REPLACE "/" "_" GIT_CACHE ${GIT_CACHE})
    string(REPLACE ".git" "" GIT_CACHE ${GIT_CACHE})
    string(REPLACE "." "_" GIT_CACHE ${GIT_CACHE})
    string(REPLACE "-" "_" GIT_CACHE ${GIT_CACHE})
    string(TOUPPER ${GIT_CACHE} GIT_CACHE)

    if (NOT "${${GIT_CACHE}}" STREQUAL "${ARG_COMMIT}${ARG_DIRECTORY}")
        message(STATUS "Cleaning repo ${ARG_DIRECTORY}")
        file(REMOVE_RECURSE ${ARG_DIRECTORY})
    endif ()
    if (NOT EXISTS ${ARG_DIRECTORY})
        message(STATUS "Cloning ${ARG_URL}")
        file(MAKE_DIRECTORY ${ARG_DIRECTORY})
        execute_process(
                COMMAND git init
                WORKING_DIRECTORY ${ARG_DIRECTORY}
                RESULT_VARIABLE GIT_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
        )
        if (NOT GIT_RESULT EQUAL 0)
            message(FATAL_ERROR "The git repository can not be initialized")
        endif ()
        execute_process(
                COMMAND git remote add origin ${ARG_URL}
                WORKING_DIRECTORY ${ARG_DIRECTORY}
                RESULT_VARIABLE GIT_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
        )
        if (NOT GIT_RESULT EQUAL 0)
            message(FATAL_ERROR "The remote origin could not be added")
        endif ()
        execute_process(
                COMMAND git fetch --depth=1 origin ${ARG_COMMIT}
                WORKING_DIRECTORY ${ARG_DIRECTORY}
                RESULT_VARIABLE GIT_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
        )
        if (NOT GIT_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to fetch from remote origin.")
        endif ()
        execute_process(
                COMMAND git reset --hard FETCH_HEAD
                WORKING_DIRECTORY ${ARG_DIRECTORY}
                RESULT_VARIABLE GIT_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
        )
        if (NOT GIT_RESULT EQUAL 0)
            message(FATAL_ERROR "Unable to reset to the fetched commit.")
        endif ()
        set(${GIT_CACHE} ${ARG_COMMIT}${ARG_DIRECTORY} CACHE STRING "Git cache installation for ${ARG_URL}" FORCE)
    endif ()
endfunction()