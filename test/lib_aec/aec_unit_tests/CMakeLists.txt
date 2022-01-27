## App name
set( APP_NAME  aec_unit_tests )

# Auto-generate task distribution scheme and top level config files

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

#copy conftest.py in the build directory since pytest_collect_file only collects tests from the directory tree where conftest.py is present
configure_file( conftest.py conftest.py COPYONLY ) 
set( GEN_SCHEDULE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../shared_src/python/generate_task_distribution_scheme.py )
set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen )
set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)

message(STATUS "${APP_NAME} aec build config:  ${AEC_UNIT_TESTS_BUILD_CONFIG}" )
message( STATUS "Unit tests speedup factor ${TEST_SPEEDUP_FACTOR}" )
set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

unset(GEN_SCHEDULE_SCRIPT_ARGS) 
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${AEC_UNIT_TESTS_BUILD_CONFIG})

file(MAKE_DIRECTORY ${AUTOGEN_DIR})

add_custom_command(
OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
COMMAND ${Python3_EXECUTABLE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
COMMENT "Generating AEC task distribution and top level config" )

## Depends on libraries
list( APPEND  DEP_LIBS_XCORE  ""  )

list( APPEND  DEP_LIBS        
    lib_aec 
    lib_de
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
)

list( APPEND  DEP_LIBS ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} )

## Compile flags
unset(COMPILE_FLAGS)
unset(COMPILE_FLAGS_XCORE)

list(APPEND   COMPILE_FLAGS_XCORE -DUNITY_SUPPORT_64 -Wno-xcore-fptrgroup )

##Linker flags
unset(LINKER_FLAGS)
list( APPEND  LINKER_FLAGS  "" )

unset(LINKER_FLAGS_XCORE)
list( APPEND  LINKER_FLAGS_XCORE  "-target=${XCORE_TARGET}"     )
list( APPEND  LINKER_FLAGS_XCORE  "-report"                     )


list( APPEND  LINKER_FLAGS ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )
list( APPEND  COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} -DSPEEDUP_FACTOR=${TEST_SPEEDUP_FACTOR} )

## Sources
# Unity
set( UNITY_PATH ${DEPS_ROOT}/Unity/src )
file( GLOB UNITY_SOURCES ${UNITY_PATH}/*.c )

# audio_test_tools
set( AUDIO_TEST_TOOLS_PATH ${DEPS_ROOT}/audio_test_tools/audio_test_tools )
list( APPEND AUDIO_TEST_TOOLS_SOURCES ${AUDIO_TEST_TOOLS_PATH}/src/testing.xc ${AUDIO_TEST_TOOLS_PATH}/src/floating_fft.xc)

#lib_dsp
set( LIB_DSP_PATH ${XCORE_SDK_PATH}/modules/lib_dsp/lib_dsp )
list( APPEND LIB_DSP_SOURCES ${LIB_DSP_PATH}/src/dsp_math.c ${LIB_DSP_PATH}/src/bfp/dsp_bfp_cls.S )

# All common sources
list( APPEND  COMMON_SOURCES ${AUTOGEN_SOURCES} ${UNITY_SOURCES} ${AUDIO_TEST_TOOLS_SOURCES} ${LIB_DSP_SOURCES} )

## Includes
list( APPEND  INCLUDES src ${SHARED_SRC_PATH}/aec ${AUTOGEN_DIR} ${UNITY_PATH} ${AUDIO_TEST_TOOLS_PATH}/api ${LIB_DSP_PATH}/api )

## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Set unity runner generate script
set( GEN_RUNNER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/generate_unity_runner.py )

# Create directory for runner files
set( RUNNERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.runners )
file( MAKE_DIRECTORY ${RUNNERS_DIR} )

file( GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.xc )

# For every source file in aec_unit_tests/src
foreach( testfile ${TEST_SOURCES} )
    unset( SOURCES )
    list( APPEND SOURCES ${COMMON_SOURCES} )
    # Get test name
    #cmake_path( GET testfile STEM TESTNAME )
    get_filename_component(TESTNAME ${testfile} NAME_WLE)
    
    # Create runner file directory
    file( MAKE_DIRECTORY ${RUNNERS_DIR}/${TESTNAME} )

    #########
    ## Create runner file
    set( RUNNER_FILE ${RUNNERS_DIR}/${TESTNAME}/${TESTNAME}_Runner.c )
    set( GEN_RUNNER_SCRIPT_BYPRODUCTS ${RUNNER_FILE} )

    unset(GEN_RUNNER_SCRIPT_ARGS) 
    list(APPEND GEN_RUNNER_SCRIPT_ARGS --project-root ${DEPS_ROOT} )
    list(APPEND GEN_RUNNER_SCRIPT_ARGS --source-file ${testfile} )
    list(APPEND GEN_RUNNER_SCRIPT_ARGS --runner-file ${RUNNER_FILE})

    ## Add command to generate runner file
    add_custom_command(
        OUTPUT ${RUNNER_FILE}
        COMMAND ${Python3_EXECUTABLE} ${GEN_RUNNER_SCRIPT} ${GEN_RUNNER_SCRIPT_ARGS}
        COMMENT "Generating AEC runner" )
    
    if (${testfile} MATCHES "test_calc_max_ref_energy.xc")
        set (SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/calc_max_ref_energy_c_wrapper.c)
    endif ()
    if (${testfile} MATCHES "test_estimate_delay.xc")
        set (SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/test_calc_fd_frame_energy.xc)
    endif()

    #########
    ## Add a build target
    add_executable( ${TESTNAME} ${testfile} ${RUNNER_FILE} ${SOURCES} )

    target_include_directories( ${TESTNAME} PRIVATE ${INCLUDES} )

    target_link_libraries( ${TESTNAME} ${DEP_LIBS})

    target_compile_options( ${TESTNAME} PRIVATE ${COMPILE_FLAGS} )
    #(because otherwise the set_target_properties command fails)
    string(REPLACE ";" " " LINKER_FLAGS_STR "${LINKER_FLAGS}")
    set_target_properties( ${TESTNAME} PROPERTIES LINK_FLAGS "${LINKER_FLAGS_STR}" )

    if ( XCORE )
        set_target_properties( ${TESTNAME} PROPERTIES
          SUFFIX ".xe"
          )
    endif()
endforeach( testfile ${TEST_SOURCES} )

