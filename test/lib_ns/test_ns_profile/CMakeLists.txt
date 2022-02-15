## App name
set( APP_NAME  test_ns_profile )

# Auto-generate task distribution scheme and top level config files

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

## Depends on libraries
unset( DEP_LIBS_XCORE )
list( APPEND  DEP_LIBS_XCORE  ""  )

unset( DEP_LIBS )
list( APPEND  DEP_LIBS        
    lib_xs3_math
    lib_ns
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
)

list( APPEND  DEP_LIBS ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} )
file( GLOB SOURCES_PROF ${CMAKE_CURRENT_SOURCE_DIR}/../../lib_adec/test_adec_profile/src/*.c )
file( GLOB SOURCES_C  ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_ns/src/*.c )
file( GLOB SOURCES_XC  ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_ns/src/*.xc )
file( GLOB SOURCES_SHARED ${SHARED_SRC_PATH}/file_utils/src/*.c )
file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${XSCOPE_FILEIO_PATH}/src/*.c )

unset( SOURCES )
list( APPEND SOURCES ${SOURCES_C} ${SOURCES_XC} ${XSCOPE_FILEIO_SOURCES} ${SOURCES_SHARED} ${SOURCES_PROF} )

unset( INCLUDES )
list( APPEND  INCLUDES 
    src 
    ${SHARED_SRC_PATH}/file_utils/src 
    ${XSCOPE_FILEIO_PATH} 
    ${XSCOPE_FILEIO_PATH}/api 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib_adec/test_adec_profile/src
)

# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG config.xscope ABSOLUTE)

## Compile flags
unset(COMPILE_FLAGS)
unset(COMPILE_FLAGS_XCORE)
list(APPEND COMPILE_FLAGS_XCORE  -DTEST_WAV_XSCOPE=1 -DPROFILE_PROCESSING=1)

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

set ( EXEC ${APP_NAME})
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
