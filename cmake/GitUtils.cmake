
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

    if (NOT "${${GIT_CACHE}}" STREQUAL "${ARG_COMMIT}${ARG_DIRECTORY}" AND EXISTS ${ARG_DIRECTORY})
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

function(GitSourceFile url ref path output_var)
    string(MD5 GIT_FILE_HASH "${url}${ref}${path}")
    set(GIT_FILE_REPO ${CMAKE_BINARY_DIR}/git_file_${GIT_FILE_HASH})
    file(REMOVE_RECURSE ${GIT_FILE_REPO})
    file(MAKE_DIRECTORY ${GIT_FILE_REPO})
    execute_process(
        COMMAND git init
        WORKING_DIRECTORY ${GIT_FILE_REPO}
        RESULT_VARIABLE GIT_RESULT_CODE
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if (GIT_RESULT_CODE EQUAL 0)
        execute_process(
            COMMAND git fetch --depth=1 --filter=blob:none ${url} ${ref}
            WORKING_DIRECTORY ${GIT_FILE_REPO}
            RESULT_VARIABLE GIT_RESULT_CODE
            OUTPUT_QUIET
            ERROR_VARIABLE GIT_ERROR
        )
    endif ()
    if (GIT_RESULT_CODE EQUAL 0)
        execute_process(
            COMMAND git show FETCH_HEAD:${path}
            WORKING_DIRECTORY ${GIT_FILE_REPO}
            RESULT_VARIABLE GIT_RESULT_CODE
            OUTPUT_VARIABLE FILE_CONTENT
            ERROR_VARIABLE GIT_ERROR
        )
    endif ()
    file(REMOVE_RECURSE ${GIT_FILE_REPO})
    set(${output_var}_RESULT ${GIT_RESULT_CODE} PARENT_SCOPE)
    set(${output_var}_ERROR "${GIT_ERROR}" PARENT_SCOPE)
    set(${output_var} "${FILE_CONTENT}" PARENT_SCOPE)
endfunction()

function(GitFile)
    cmake_parse_arguments(ARG "" "URL;DIRECTORY;OUTPUT_VARIABLE" "" ${ARGN})
    if (NOT ARG_DIRECTORY AND NOT ARG_OUTPUT_VARIABLE)
        message(FATAL_ERROR "GitFile requires either DIRECTORY or OUTPUT_VARIABLE.")
    endif ()
    set(GIT_SOURCE FALSE)
    if ("${ARG_URL}" MATCHES "^(https://[^/]*googlesource\\.com/.+)/\\+/(refs/[a-z]+/[^/]+|[0-9a-f]+)/(.+)$")
        set(GIT_SOURCE TRUE)
        set(GIT_SOURCE_URL ${CMAKE_MATCH_1})
        set(GIT_SOURCE_REF ${CMAKE_MATCH_2})
        set(GIT_SOURCE_PATH ${CMAKE_MATCH_3})
    elseif ("${ARG_URL}" MATCHES "github.com/.+/blob/")
        string(REPLACE "github.com" "raw.githubusercontent.com" ARG_URL "${ARG_URL}")
        string(REPLACE "/blob/" "/" ARG_URL "${ARG_URL}")
    endif ()
    set(GIT_RESULT_CODE 1)
    set(GIT_ATTEMPT 0)
    while(NOT GIT_RESULT_CODE EQUAL 0 AND GIT_ATTEMPT LESS 20)
        if(GIT_ATTEMPT GREATER 0)
            execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 5)
        endif ()
        if (GIT_SOURCE)
            GitSourceFile(${GIT_SOURCE_URL} ${GIT_SOURCE_REF} ${GIT_SOURCE_PATH} FILE_CONTENT)
            set(GIT_RESULT_CODE ${FILE_CONTENT_RESULT})
            set(GIT_ERROR "${FILE_CONTENT_ERROR}")
        else ()
            execute_process(
                COMMAND curl -sSL --fail ${ARG_URL}
                RESULT_VARIABLE GIT_RESULT_CODE
                OUTPUT_VARIABLE FILE_CONTENT
                ERROR_VARIABLE GIT_ERROR
            )
        endif ()
        if(GIT_RESULT_CODE EQUAL 0 AND "${FILE_CONTENT}" STREQUAL "")
            set(GIT_RESULT_CODE 1)
        endif ()
        math(EXPR GIT_ATTEMPT "${GIT_ATTEMPT} + 1")
    endwhile ()
    if(NOT GIT_RESULT_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to fetch ${ARG_URL} after ${GIT_ATTEMPT} attempts: ${GIT_ERROR}")
    endif ()
    if (ARG_OUTPUT_VARIABLE)
        set(${ARG_OUTPUT_VARIABLE} "${FILE_CONTENT}" PARENT_SCOPE)
    else ()
        file(WRITE ${ARG_DIRECTORY} "${FILE_CONTENT}")
    endif ()
endfunction()
