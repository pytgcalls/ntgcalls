function(base64_decode input output_var)
    set(alpha "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")

    string(REPLACE "\n" "" input "${input}")
    string(REPLACE "\r" "" input "${input}")
    string(REPLACE " "  "" input "${input}")

    string(LENGTH "${input}" len)
    set(result "")
    set(i 0)

    while(i LESS ${len})
        string(SUBSTRING "${input}" ${i} 1 c1)
        math(EXPR i1 "${i}+1")
        math(EXPR i2 "${i}+2")
        math(EXPR i3 "${i}+3")
        string(SUBSTRING "${input}" ${i1} 1 c2)
        string(SUBSTRING "${input}" ${i2} 1 c3)
        string(SUBSTRING "${input}" ${i3} 1 c4)

        string(FIND "${alpha}" "${c1}" v1)
        string(FIND "${alpha}" "${c2}" v2)
        string(FIND "${alpha}" "${c3}" v3)
        string(FIND "${alpha}" "${c4}" v4)

        math(EXPR b1 "(${v1} << 2) | (${v2} >> 4)")
        string(ASCII ${b1} ch1)
        string(APPEND result "${ch1}")

        if(NOT c3 STREQUAL "=")
            math(EXPR b2 "((${v2} & 0xF) << 4) | (${v3} >> 2)")
            string(ASCII ${b2} ch2)
            string(APPEND result "${ch2}")
        endif()

        if(NOT c4 STREQUAL "=")
            math(EXPR b3 "((${v3} & 0x3) << 6) | ${v4}")
            string(ASCII ${b3} ch3)
            string(APPEND result "${ch3}")
        endif()

        math(EXPR i "${i}+4")
    endwhile()

    set(${output_var} "${result}" PARENT_SCOPE)
endfunction()