# Writes a version into one of docsdata's config.xml options, so the documentation
# never states a version by hand.
#   cmake -DCONFIG_XML=<docsdata>/config.xml -DOPTION=NTGCALLS_VERSION \
#         -DVERSION=3.0.0b18 -P StampDocsVersion.cmake

foreach(_v CONFIG_XML OPTION VERSION)
    if(NOT DEFINED ${_v})
        message(FATAL_ERROR "docs: ${_v} is required")
    endif()
endforeach()
if(NOT EXISTS "${CONFIG_XML}")
    message(FATAL_ERROR "docs: config not found: ${CONFIG_XML}")
endif()

# a local build carries a "+dev.<sha>" suffix that means nothing to a reader
string(REGEX REPLACE "\\+.*$" "" _version "${VERSION}")

file(READ "${CONFIG_XML}" _cfg)
set(_pattern "<option id=\"${OPTION}\">[^<]*</option>")
if(NOT _cfg MATCHES "${_pattern}")
    message(FATAL_ERROR "docs: option ${OPTION} not found in ${CONFIG_XML}")
endif()
string(REGEX REPLACE "${_pattern}" "<option id=\"${OPTION}\">${_version}</option>" _cfg "${_cfg}")
file(WRITE "${CONFIG_XML}" "${_cfg}")
message(STATUS "docs: ${OPTION} -> ${_version}")
