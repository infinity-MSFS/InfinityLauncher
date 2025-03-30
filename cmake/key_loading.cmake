function(load_keys)
    # Build Bin2Header first
    message("${Blue}Building Bin2Header tool${ColorReset}")

    # First ensure Bin2Header is built before proceeding
    add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Bin2Header${CMAKE_EXECUTABLE_SUFFIX}"
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Bin2Header
            COMMENT "Building Bin2Header tool"
    )

    # Create a custom target that depends on the Bin2Header executable
    add_custom_target(EnsureBin2HeaderExists
            DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Bin2Header${CMAKE_EXECUTABLE_SUFFIX}"
    )

    # Set the converter path to the built Bin2Header executable
    set(CONVERTER "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Bin2Header${CMAKE_EXECUTABLE_SUFFIX}")

    set(KEY_PAIRS
            "client_key"
    )

    set(GENERATED_HEADERS "")

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/keys/include")

    foreach (key_name ${KEY_PAIRS})
        set(KEY_BIN "${CMAKE_CURRENT_SOURCE_DIR}/keys/${key_name}.bin")
        set(KEY_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/keys/include/${key_name}.h")

        if (NOT EXISTS "${KEY_BIN}")
            message(WARNING "Binary key file not found: ${KEY_BIN}")
            continue()
        endif ()

        add_custom_command(
                OUTPUT "${KEY_HEADER}"
                COMMAND "${CONVERTER}" -o "${KEY_HEADER}" -n "${key_name}" "${KEY_BIN}"
                DEPENDS EnsureBin2HeaderExists "${KEY_BIN}"
                COMMENT "Generating header for ${key_name}"
                VERBATIM
        )

        list(APPEND GENERATED_HEADERS "${KEY_HEADER}")
    endforeach ()

    add_custom_target(GenerateKeyHeaders ALL DEPENDS ${GENERATED_HEADERS})

    set_property(GLOBAL PROPERTY KEYS_LOADED TRUE)

    list(LENGTH GENERATED_HEADERS num_generated)
    list(LENGTH KEY_PAIRS total_keys)
    message("${Blue}Processed ${Green}${num_generated}${Blue} of ${Green}${total_keys}${Blue} keys${ColorReset}")
endfunction()