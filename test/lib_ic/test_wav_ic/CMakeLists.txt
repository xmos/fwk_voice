## App name
set( APP_NAME  test_wav_ic )

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()



## Depends on libraries
unset( DEP_LIBS_XCORE )
list( APPEND  DEP_LIBS_XCORE  ""  )

unset( DEP_LIBS )
list( APPEND  DEP_LIBS        
    lib_xs3_math
    lib_ic
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
)

list( APPEND  DEP_LIBS ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} )

## Sources
file( GLOB SOURCES_C  ${CMAKE_CURRENT_SOURCE_DIR}/../../../examples/bare-metal/ic/src//*.c )
file( GLOB SOURCES_XC  ${CMAKE_CURRENT_SOURCE_DIR}/../../../examples/bare-metal/ic/src/*.xc )
file( GLOB SOURCES_IC_PROCESS_FRAME  ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/ic/*.c )
file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${DEPS_ROOT}/xscope_fileio/xscope_fileio/src/*.c )
file( GLOB_RECURSE AUDIO_TEST_TOOLS_SOURCES ${DEPS_ROOT}/audio_test_tools/audio_test_tools/src/burners.S )
file( GLOB SOURCES_FILE_UTILS ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/file_utils/*.c ) 

unset( SOURCES )
list( APPEND SOURCES ${SOURCES_C} ${SOURCES_IC_PROCESS_FRAME} ${SOURCES_XC} ${SOURCES_FILE_UTILS})

if ( XCORE )
    list( APPEND SOURCES ${XSCOPE_FILEIO_SOURCES}  ${AUDIO_TEST_TOOLS_SOURCES})
endif()


unset( INCLUDES )
list( APPEND  INCLUDES 
    src 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../examples/bare-metal/shared_src/ic 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../examples/bare-metal/shared_src/file_utils 
    ${DEPS_ROOT}/xscope_fileio/xscope_fileio 
    ${DEPS_ROOT}/xscope_fileio/xscope_fileio/api 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib_aec/test_aec_profile/src
    )

# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG config.xscope ABSOLUTE)

## Compile flags
unset(COMPILE_FLAGS)
unset(COMPILE_FLAGS_XCORE)
list(APPEND COMPILE_FLAGS_XCORE  -DTEST_WAV_XSCOPE=1)

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

set ( EXEC ${APP_NAME}_${CONFIG_STR} )
add_executable( ${EXEC} ${SOURCES} )

target_include_directories( ${EXEC} PRIVATE ${INCLUDES})

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
