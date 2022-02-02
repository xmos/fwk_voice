## App name
set( APP_NAME  pipeline_single_threaded )

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

## Depends on libraries
list( APPEND  DEP_LIBS_XCORE  ""  )
list( APPEND  DEP_LIBS_Linux  m   )

list( APPEND  DEP_LIBS        
    lib_aec
    lib_noise_suppression
    lib_agc
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
)

## Sources
file( GLOB_RECURSE SOURCES_C  src/*.c )
file( GLOB SOURCES_SHARED_FILEIO ${SHARED_SRC_PATH}/file_utils/*.c )
file( GLOB SOURCES_SHARED_AEC ${SHARED_SRC_PATH}/aec/aec_process_frame_1thread.c )
unset( SOURCES_SHARED )
list( APPEND SOURCES_SHARED ${SOURCES_SHARED_FILEIO} ${SOURCES_SHARED_AEC} )
file( GLOB SOURCES_XC src/*.xc )
file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES ${XSCOPE_FILEIO_PATH}/src/*.c )

unset( SOURCES )
list( APPEND  SOURCES ${SOURCES_C} ${SOURCES_SHARED})
unset( SOURCES_XCORE )
list( APPEND SOURCES_XCORE ${SOURCES_XC} ${XSCOPE_FILEIO_SOURCES} )
list( APPEND SOURCES ${SOURCES_${CMAKE_SYSTEM_NAME}} )

## Includes
unset( INCLUDES )
list( APPEND  INCLUDES src ${SHARED_SRC_PATH}/file_utils ${SHARED_SRC_PATH}/aec )
unset( INCLUDES_XCORE )
list( APPEND INCLUDES_XCORE ${XSCOPE_FILEIO_PATH} ${XSCOPE_FILEIO_PATH}/api )
list( APPEND INCLUDES ${INCLUDES_${CMAKE_SYSTEM_NAME}} )

# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG config.xscope ABSOLUTE)

## Compile flags
unset(COMPILE_FLAGS)
unset(COMPILE_FLAGS_XCORE)

list(APPEND   COMPILE_FLAGS_XCORE  -DTEST_WAV_XSCOPE=1 )

unset(COMPILE_FLAGS_Linux)
list( APPEND  COMPILE_FLAGS_Linux   ""  )


##Linker flags
unset(LINKER_FLAGS)
list( APPEND  LINKER_FLAGS  "" )

unset(LINKER_FLAGS_XCORE)
list( APPEND  LINKER_FLAGS_XCORE  "-target=${XCORE_TARGET}"     )
list( APPEND  LINKER_FLAGS_XCORE  "-report"                     )
list( APPEND  LINKER_FLAGS_XCORE  "${XSCOPE_CONFIG}"            )

unset(LINKER_FLAGS_Linux)
list( APPEND  LINKER_FLAGS_Linux  "" )

list( APPEND  LINKER_FLAGS ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )
list( APPEND  COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )

#########
## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable( ${APP_NAME} ${SOURCES} ${SOURCES_C_APP} )

target_include_directories( ${APP_NAME} PRIVATE ${INCLUDES} )

target_link_libraries( ${APP_NAME} ${DEP_LIBS})

target_compile_options( ${APP_NAME} PRIVATE ${COMPILE_FLAGS} )

#(because otherwise the set_target_properties command fails)
string(REPLACE ";" " " LINKER_FLAGS_STR "${LINKER_FLAGS}")
set_target_properties( ${APP_NAME} PROPERTIES LINK_FLAGS "${LINKER_FLAGS_STR}" )

if ( XCORE )
  set_target_properties( ${APP_NAME} PROPERTIES
      SUFFIX ".xe"
      LINK_DEPENDS  ${XSCOPE_CONFIG}
      )
endif()
