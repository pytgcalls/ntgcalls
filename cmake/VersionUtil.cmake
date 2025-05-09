execute_process(
    COMMAND git config --global --add safe.directory ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REPLACE "." ";" VERSION_PARTS ${PROJECT_VERSION})
list(GET VERSION_PARTS 0 MAJOR)
list(GET VERSION_PARTS 1 MINOR)
list(GET VERSION_PARTS 2 PATCH)
list(LENGTH VERSION_PARTS VERSION_PARTS_LENGTH)

if (ANDROID_TAGS OR GITHUB_TAGS)
    set(ALPHA_TAG "-alpha")
    set(BETA_TAG "-beta")
    set(RELEASE_CANDIDATE_TAG "-rc")
else ()
    set(ALPHA_TAG "a")
    set(BETA_TAG "b")
    set(RELEASE_CANDIDATE_TAG "rc")
endif ()

if(VERSION_PARTS_LENGTH EQUAL 4)
    list(GET VERSION_PARTS 3 EXTRA)

    if (ANDROID_TAGS OR GITHUB_TAGS)
        if(EXTRA LESS 10)
            set(EXTRA "0${EXTRA}")
        else()
            set(EXTRA "${EXTRA}")
        endif()
    endif ()

    set(VERSION_TAG "${BETA_TAG}")

    if(GIT_BRANCH MATCHES "^.*/")
        set(VERSION_TAG "${ALPHA_TAG}")
    elseif (GIT_BRANCH MATCHES "^master")
        set(VERSION_TAG "${RELEASE_CANDIDATE_TAG}")
    endif ()

    set(VERSION_NAME "${MAJOR}.${MINOR}.${PATCH}${VERSION_TAG}${EXTRA}")
else()
    set(VERSION_NAME "${MAJOR}.${MINOR}.${PATCH}")
endif()

if (NOT ANDROID_TAGS AND NOT GITHUB_TAGS)
    if(GIT_BRANCH MATCHES "^.*/")
        set(VERSION_NAME "${VERSION_NAME}+canary.${GIT_COMMIT}")
    elseif(GIT_BRANCH MATCHES "^dev")
        set(VERSION_NAME "${VERSION_NAME}+dev.${GIT_COMMIT}")
    endif()
endif ()

if(DEFINED CMAKE_PARENT_LIST_FILE)
    add_compile_definitions(NTG_VERSION="${VERSION_NAME}")
else ()
    message("${VERSION_NAME}")
endif ()