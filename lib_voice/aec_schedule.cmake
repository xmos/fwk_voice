
if (AEC_SCHEDULE_CONFIG)

find_program(PYTHON_EXE python NO_CACHE)

message(STATUS "PYTHON_EXE = ${PYTHON_EXE}")
message(STATUS "AEC_SCHEDULE_CONFIG = ${AEC_SCHEDULE_CONFIG}")


# Resolve script path relative to this file, to avoid depending on CMAKE_SOURCE_DIR
get_filename_component(GEN_SCHEDULE_SCRIPT
  "${XMOS_SANDBOX_DIR}/fwk_voice/test/shared/python/generate_task_distribution_scheme.py"
  REALPATH
)
if (NOT EXISTS "${GEN_SCHEDULE_SCRIPT}")
  message(FATAL_ERROR "AEC schedule generator not found at: ${GEN_SCHEDULE_SCRIPT}")
endif()
if(AEC_SCHEDULE_AUTOGEN_DIR)
  set(AUTOGEN_DIR "${AEC_SCHEDULE_AUTOGEN_DIR}")
else()
  set(AUTOGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/src.autogen")
endif()
set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_conf.h)

set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

unset(GEN_SCHEDULE_SCRIPT_ARGS)
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${AEC_SCHEDULE_CONFIG})

add_custom_command(
    OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
    COMMAND ${PYTHON_EXE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
    COMMENT "Generating task distribution and top level config for schedule ${AEC_SCHEDULE_CONFIG}"
    DEPENDS ${GEN_SCHEDULE_SCRIPT} )

endif()
