# Splices the generated sidebar groups into the marked block of docsdata's config.xml.
#   cmake -DCONFIG_XML=<docsdata>/config.xml -DINDEX_FILE=<docs out>/files-list.xml \
#         -P UpdateDocsIndex.cmake

if(NOT DEFINED CONFIG_XML OR NOT DEFINED INDEX_FILE)
    message(FATAL_ERROR "docs: CONFIG_XML and INDEX_FILE are required")
endif()
foreach(_f "${CONFIG_XML}" "${INDEX_FILE}")
    if(NOT EXISTS "${_f}")
        message(FATAL_ERROR "docs: file not found: ${_f}")
    endif()
endforeach()

set(_start "<!--GENERATED INDEX START-->")
set(_end "<!--GENERATED INDEX END-->")

file(READ "${CONFIG_XML}" _cfg)
file(READ "${INDEX_FILE}" _index)

string(FIND "${_cfg}" "${_start}" _a)
string(FIND "${_cfg}" "${_end}" _b)
if(_a LESS 0 OR _b LESS 0 OR _b LESS _a)
    message(FATAL_ERROR "docs: index markers not found in ${CONFIG_XML}")
endif()

string(LENGTH "${_start}" _slen)
math(EXPR _head "${_a}+${_slen}")
string(SUBSTRING "${_cfg}" 0 ${_head} _before)
string(SUBSTRING "${_cfg}" ${_b} -1 _after)
file(WRITE "${CONFIG_XML}" "${_before}\n${_index}    ${_after}")
message(STATUS "docs: index updated in ${CONFIG_XML}")
