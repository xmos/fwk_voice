
function(generate_schedule TARGET AEC_SCHEDULE_CONFIG)
    find_program(PYTHON_EXE python NO_CACHE)
    if( NOT PYTHON_EXE )
        message(FATAL_ERROR "Python not found for running . ")
    endif()

    set( GEN_SCHEDULE_SCRIPT "${CMAKE_SOURCE_DIR}/test/shared/python/generate_task_distribution_scheme.py" )
    set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_src.autogen )
    set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
    set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_conf.h)

    set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

    unset(GEN_SCHEDULE_SCRIPT_ARGS)
    list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
    list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${AEC_SCHEDULE_CONFIG})

    add_custom_command(
        OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
        BYPRODUCTS ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
        COMMAND ${PYTHON_EXE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
        COMMENT "Generating target ${TARGET} task distribution and top level config"
        DEPENDS ${GEN_SCHEDULE_SCRIPT}
        VERBATIM
        COMMAND_EXPAND_LISTS)


    target_sources(${TARGET} PRIVATE ${AUTOGEN_SOURCES})

    target_include_directories(${TARGET} PRIVATE ${AUTOGEN_DIR})

    target_compile_definitions(${TARGET} PRIVATE __aec_conf_h_exists__)

endfunction()

