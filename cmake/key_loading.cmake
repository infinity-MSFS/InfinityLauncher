function(add_inline_to_headers)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "HEADERS")

    foreach (header ${ARG_HEADERS})
        file(READ ${header} CONTENT)

        string(REGEX REPLACE "(^|\n)unsigned" "\\1inline unsigned" MODIFIED_CONTENT "${CONTENT}")

        file(WRITE ${header} "${MODIFIED_CONTENT}")
    endforeach ()
endfunction()

function(load_keys)
    # Client
    set(CLIENT_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/client_key.bin")
    set(CLIENT_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/client_key.h")
    execute_process(
            COMMAND xxd -n "admin_key" -i ${CLIENT_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${CLIENT_KEY_HEADER}" "${xxd_output}")
    # Admin
    set(ADMIN_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/admin_key.bin")
    set(ADMIN_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/admin_key.h")
    execute_process(
            COMMAND xxd -n "admin_key" -i ${ADMIN_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${ADMIN_KEY_HEADER}" "${xxd_output}")
    # Aero
    set(AERO_DYNAMICS_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/aero_dynamics_key.bin")
    set(AERO_DYNAMICS_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/aero_dynamics_key.h")
    execute_process(
            COMMAND xxd -n "aero_dynamics_key" -i ${AERO_DYNAMICS_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${AERO_DYNAMICS_KEY_HEADER}" "${xxd_output}")
    set(DELTA_SIM_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/delta_sim_key.bin")
    set(DELTA_SIM_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/delta_sim_key.h")
    execute_process(
            COMMAND xxd -n "delta_sim_key" -i ${DELTA_SIM_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${DELTA_SIM_KEY_HEADER}" "${xxd_output}")
    # Group
    set(GROUP_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/group_key.bin")
    set(GROUP_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/group_key.h")
    execute_process(
            COMMAND xxd -n "group_key" -i ${GROUP_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${GROUP_KEY_HEADER}" "${xxd_output}")
    # Lunar Sim
    set(LUNAR_SIM_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/lunar_sim_key.bin")
    set(LUNAR_SIM_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/lunar_sim_key.h")
    execute_process(
            COMMAND xxd -n "lunar_sim_key" -i ${LUNAR_SIM_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${LUNAR_SIM_KEY_HEADER}" "${xxd_output}")
    # Ouroboros
    set(OUROBOROS_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/ouroboros_key.bin")
    set(OUROBOROS_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/ouroboros_key.h")
    execute_process(
            COMMAND xxd -n "ouroboros_key" -i ${OUROBOROS_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${OUROBOROS_KEY_HEADER}" "${xxd_output}")
    # Qbit Sim
    set(QBIT_SIM_KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/qbit_sim_key.bin")
    set(QBIT_SIM_KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/qbit_sim_key.h")
    execute_process(
            COMMAND xxd -n "qbit_sim_key" -i ${QBIT_SIM_KEY_BIN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE xxd_result
            OUTPUT_VARIABLE xxd_output
            ERROR_VARIABLE xxd_error
    )
    file(WRITE "${QBIT_SIM_KEY_HEADER}" "${xxd_output}")

    add_inline_to_headers(HEADERS ${CLIENT_KEY_HEADER} ${ADMIN_KEY_HEADER} ${AERO_DYNAMICS_KEY_HEADER} ${DELTA_SIM_KEY_HEADER} ${GROUP_KEY_HEADER} ${LUNAR_SIM_KEY_HEADER} ${OUROBOROS_KEY_HEADER} ${QBIT_SIM_KEY_HEADER})
endfunction()