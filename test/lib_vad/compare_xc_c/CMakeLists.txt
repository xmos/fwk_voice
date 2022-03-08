## This cmake file builds the "old vad" testwav example. Note inclusion of lots
## of source files from the old repos which do not support cmake libs
## App name
set( APP_NAME  avona_run_old_vad )


## Sources
file( GLOB_RECURSE SOURCES_C  src/*.c )
file( GLOB_RECURSE SOURCES_XC  src/*.xc )

## LIB_AI
file( GLOB_RECURSE LIB_AI_SOURCES_C  ${TEST_ONLY_DEPS_ROOT}/lib_ai/lib_ai/src/*.c )
file( GLOB_RECURSE LIB_AI_SOURCES_XC  ${TEST_ONLY_DEPS_ROOT}/lib_ai/lib_ai/src/*.xc )
file( GLOB_RECURSE LIB_AI_SOURCES_ASM  ${TEST_ONLY_DEPS_ROOT}/lib_ai/lib_ai/src/*.S )
set( LIB_AI_SOURCES  ${LIB_AI_SOURCES_C} ${LIB_AI_SOURCES_XC} ${LIB_AI_SOURCES_ASM})
set( LIB_AI_INCLUDES ${TEST_ONLY_DEPS_ROOT}/lib_ai/lib_ai/api/ ${TEST_ONLY_DEPS_ROOT}/lib_ai/lib_ai/src/)

## LIB_VAD
file( GLOB_RECURSE LIB_VAD_SOURCES_C  ${TEST_ONLY_DEPS_ROOT}/lib_vad/lib_vad/src/*.c )
file( GLOB_RECURSE LIB_VAD_SOURCES_XC  ${TEST_ONLY_DEPS_ROOT}/lib_vad/lib_vad/src/*.xc )
file( GLOB_RECURSE LIB_VAD_SOURCES_ASM  ${TEST_ONLY_DEPS_ROOT}/lib_vad/lib_vad/src/*.S )
set( LIB_VAD_SOURCES  ${LIB_VAD_SOURCES_C} ${LIB_VAD_SOURCES_XC} ${LIB_VAD_SOURCES_ASM})
list(REMOVE_ITEM LIB_VAD_SOURCES ${TEST_ONLY_DEPS_ROOT}/lib_vad/lib_vad/src/vad_test_task.xc)

set(LIB_ASM_SOURCES ${LIB_AI_SOURCES_ASM})
list(APPEND LIB_ASM_SOURCES ${LIB_VAD_SOURCES_ASM})
## cmake doesn't recognize .S files as assembly by default
set_source_files_properties(LIB_ASM_SOURCES PROPERTIES LANGUAGE ASM)

set( LIB_VAD_INCLUDES ${TEST_ONLY_DEPS_ROOT}/lib_vad/lib_vad/api/ ${TEST_ONLY_DEPS_ROOT}/lib_vad/lib_vad/src/)

#################################

add_executable(${APP_NAME})

target_sources(${APP_NAME}
    PRIVATE
        ${SOURCES_C}
        ${SOURCES_XC}
        ${LIB_AI_SOURCES}
        ${LIB_VAD_SOURCES}
        )

target_include_directories(${APP_NAME}
    PRIVATE
        src
        ${LIB_AI_INCLUDES} 
        ${LIB_VAD_INCLUDES}
        )

target_link_libraries(${APP_NAME}
    PUBLIC
        avona_test_deps::lib_dsp
        sdk::lib_xs3_math
        avona::test::shared::test_utils
        avona::example::fileutils
        )

target_compile_options(${APP_NAME}
    PRIVATE "-target=${XCORE_TARGET}")

target_link_options(${APP_NAME}
    PRIVATE
        "-target=${XCORE_TARGET}"
        "-report"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
