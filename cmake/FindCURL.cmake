execute_process(
    COMMAND curl --version
    RESULT_VARIABLE CURL_RESULT
    OUTPUT_VARIABLE CURL_VERSION
    ERROR_QUIET
)
string(REGEX MATCH "curl ([0-9]+\\.[0-9]+\\.[0-9]+)" CURL_VERSION_MATCH "${CURL_VERSION}")
if(CURL_RESULT EQUAL 0)
    message(STATUS "curl v${CMAKE_MATCH_1}")
else()
    message(FATAL_ERROR "Curl not found. Please install curl.")
endif()