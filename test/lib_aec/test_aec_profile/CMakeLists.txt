## App name
set( APP_NAME  test_aec_profile )

# Auto-generate task distribution scheme and top level config files

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

set( GEN_SCHEDULE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../shared_src/python/generate_task_distribution_scheme.py )

## Build each config
foreach( BUILD_CONFIG ${TEST_AEC_PROFILE_BUILD_CONFIG} )
    # Convert to config str of the form <threads>_<ychannels>_<xchannels>_<mainphases>_<shadowphases>
    string( REPLACE " " "_" CONFIG_STR ${BUILD_CONFIG} )
    message( STATUS "${APP_NAME} ${CONFIG_STR}" )
    
    ## Generate schedule for given config
    set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen_${CONFIG_STR} )
    set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
    set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)
    set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )
    unset( GEN_SCHEDULE_SCRIPT_ARGS)
    list( APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
    list( APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${BUILD_CONFIG})
    file( MAKE_DIRECTORY ${AUTOGEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
        COMMAND ${Python3_EXECUTABLE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
        COMMENT "Generating AEC task distribution and top level config" )

    ##
    
    ## Depends on libraries
    unset( DEP_LIBS_XCORE )
    list( APPEND  DEP_LIBS_XCORE  ""  )
    
    unset( DEP_LIBS )
    list( APPEND  DEP_LIBS        
        lib_xs3_math
        lib_aec 
        ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
    )

    list( APPEND  DEP_LIBS ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} )

    ## Sources
    file( GLOB SOURCES_PROF  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c )
    file( GLOB SOURCES_C  ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_aec/src/*.c )
    file( GLOB SOURCES_XC  ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_aec/src/*.xc )
if ( XCORE )
    file( GLOB SOURCES_AEC_PROCESS_FRAME  ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/aec/*.c )
else()
    ## Only 1 thread process_frame() builds for x86
    file( GLOB SOURCES_AEC_PROCESS_FRAME ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/aec/aec_process_frame_1thread.c )
endif()
    file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${DEPS_ROOT}/xscope_fileio/xscope_fileio/src/*.c )
    file( GLOB_RECURSE AUDIO_TEST_TOOLS_SOURCES ${DEPS_ROOT}/audio_test_tools/audio_test_tools/src/burners.S )
    file( GLOB SOURCES_FILE_UTILS ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/file_utils/*.c ) 
    
    unset( SOURCES )
    list( APPEND  SOURCES ${SOURCES_C} ${SOURCES_AEC_PROCESS_FRAME} ${SOURCES_XC} ${XSCOPE_FILEIO_SOURCES} ${SOURCES_FILE_UTILS} ${AUDIO_TEST_TOOLS_SOURCES} ${SOURCES_PROF} ${AUTOGEN_SOURCES})
    unset( INCLUDES )
    list( APPEND  INCLUDES src ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/aec ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/file_utils ${DEPS_ROOT}/xscope_fileio/xscope_fileio ${DEPS_ROOT}/xscope_fileio/xscope_fileio/api ${CMAKE_CURRENT_SOURCE_DIR}/src/ ${AUTOGEN_DIR})

    # set( XSCOPE_CONFIG config.xscope )
    get_filename_component(XSCOPE_CONFIG config.xscope ABSOLUTE)

    ## Compile flags
    unset(COMPILE_FLAGS)
    unset(COMPILE_FLAGS_XCORE)
    list(APPEND   COMPILE_FLAGS_XCORE  -DTEST_WAV_XSCOPE=1 -DPROFILE_PROCESSING=1)

    ##Linker flags
    unset(LINKER_FLAGS)
    list( APPEND  LINKER_FLAGS  "" )

    unset(LINKER_FLAGS_XCORE)
    list( APPEND  LINKER_FLAGS_XCORE  "-target=${XCORE_TARGET}"     )
    list( APPEND  LINKER_FLAGS_XCORE  "-report"                     )
    list( APPEND  LINKER_FLAGS_XCORE  "${XSCOPE_CONFIG}"            )


    list( APPEND  LINKER_FLAGS ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )
    list( APPEND  COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )

    #########
    ## executable output directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

    ## 1 thread executable
    set ( EXEC ${APP_NAME}_${CONFIG_STR} )
    add_executable( ${EXEC} ${SOURCES} )

    target_include_directories( ${EXEC} PRIVATE ${INCLUDES} ${AUTOGEN_DIR})

    target_link_libraries( ${EXEC} ${DEP_LIBS})

    target_compile_options( ${EXEC} PRIVATE ${COMPILE_FLAGS} )

    #(because otherwise the set_target_properties command fails)
    string(REPLACE ";" " " LINKER_FLAGS_STR "${LINKER_FLAGS}")
    set_target_properties( ${EXEC} PROPERTIES LINK_FLAGS "${LINKER_FLAGS_STR}" )

    if ( XCORE )
        set_target_properties( ${EXEC} PROPERTIES
        SUFFIX ".xe"
        LINK_DEPENDS  ${XSCOPE_CONFIG}
        )
    endif()
endforeach( BUILD_CONFIG ${TEST_CONFIG} )
