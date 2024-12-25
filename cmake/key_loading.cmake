function(add_inline_to_headers)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "HEADERS")

    foreach (header ${ARG_HEADERS})
        file(READ ${header} CONTENT)

        string(REGEX REPLACE "(^|\n)unsigned" "\\1inline unsigned" MODIFIED_CONTENT "${CONTENT}")

        file(WRITE ${header} "${MODIFIED_CONTENT}")
    endforeach ()
endfunction()

function(load_keys)
    set(KEY_PAIRS
            "client_key"
    )

    set(GENERATED_HEADERS "")

    if (WIN32)
        set(CONVERTER "${CMAKE_CURRENT_SOURCE_DIR}/keys/bin2header.exe")
    else ()
        find_program(XXD_EXECUTABLE xxd REQUIRED)
        set(CONVERTER ${XXD_EXECUTABLE})
    endif ()

    foreach (key_name ${KEY_PAIRS})
        set(KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/${key_name}.bin")
        set(KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/${key_name}.h")

        if (NOT EXISTS "${KEY_BIN}")
            message(WARNING "Binary key file not found: ${KEY_BIN}")
            continue()
        endif ()

        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/keys/include")

        if (WIN32)
            execute_process(
                    COMMAND "${CONVERTER}" -o "${KEY_HEADER}" -n "${key_name}" "${KEY_BIN}"
                    RESULT_VARIABLE convert_result
                    ERROR_VARIABLE convert_error
                    OUTPUT_VARIABLE convert_output
            )
        else ()
            execute_process(
                    COMMAND ${CONVERTER} -n "${key_name}" -i "${KEY_BIN}"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE convert_result
                    ERROR_VARIABLE convert_error
                    OUTPUT_VARIABLE convert_output
            )
            if (convert_result EQUAL 0)
                file(WRITE "${KEY_HEADER}" "${convert_output}")
            endif ()
            execute_process(
                    COMMAND /bin/sh -c "sed -i '1s/^/#pragma once\\n/' ${KEY_HEADER} && sed -i 's/^unsigned /inline unsigned /' ${KEY_HEADER}"
            )

        endif ()

        if (convert_result EQUAL 0)
            list(APPEND GENERATED_HEADERS "${KEY_HEADER}")
            message("${Blue}Successfully processed: ${Green}${key_name}${ColorReset}")
        else ()
            message(WARNING "Failed to process ${key_name}: ${convert_error}")
        endif ()
    endforeach ()

    list(LENGTH GENERATED_HEADERS num_generated)
    list(LENGTH KEY_PAIRS total_keys)
    message("${Blue}Processed ${Green}${num_generated}${Blue} of ${Green}${total_keys}${Blue} keys${ColorReset}")
endfunction()