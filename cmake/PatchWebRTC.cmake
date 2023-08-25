set(WEBRTC_PATCH_LOCATION ${WEBRTC_DIR}/patches/${WEBRTC_PATCH_FILE})
if(NOT DEFINED LAST_REVISION OR NOT ${LAST_REVISION} STREQUAL ${WEBRTC_REVISION} OR NOT EXISTS ${WEBRTC_PATCH_LOCATION})
    message(STATUS "Downloading patch file for ${WEBRTC_REVISION} revision")
    execute_process(
        COMMAND curl -s ${WEBRTC_PATCH_URL}
        RESULT_VARIABLE PATCH_RESULT_CODE
        OUTPUT_VARIABLE PATCH_CONTENT_BASE64
    )
    if(NOT PATCH_RESULT_CODE EQUAL 0)
        message(FATAL_ERROR "Error while downloading patch file")
    endif ()
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "import base64; import sys; sys.stdout.buffer.write(base64.b64decode('${PATCH_CONTENT_BASE64}'))"
        OUTPUT_VARIABLE PATCH_CONTENT
    )
    set(LAST_REVISION ${WEBRTC_REVISION} CACHE STRING "Last patch revision" FORCE)
    file(WRITE ${WEBRTC_PATCH_LOCATION} "${PATCH_CONTENT}")
endif ()