if(NOT DEFINED LAST_REVISION OR
        NOT ${LAST_REVISION} STREQUAL ${WEBRTC_REVISION} OR
        NOT EXISTS ${WEBRTC_INCLUDE}/${WEBRTC_PATCH_FILE})
    set(LAST_REVISION ${WEBRTC_REVISION} CACHE STRING "Last patch revision" FORCE)
    message(STATUS "Downloading patch file for ${LAST_REVISION} revision")
    execute_process(
        COMMAND curl -s ${WEBRTC_PATCH_URL}
        RESULT_VARIABLE PATCH_RESULT_CODE
        OUTPUT_VARIABLE PATCH_CONTENT_BASE64
    )
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "import base64; import sys; sys.stdout.buffer.write(base64.b64decode('${PATCH_CONTENT_BASE64}'))"
        OUTPUT_VARIABLE PATCH_CONTENT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    file(WRITE ${WEBRTC_INCLUDE}/${WEBRTC_PATCH_FILE} ${PATCH_CONTENT})
endif ()