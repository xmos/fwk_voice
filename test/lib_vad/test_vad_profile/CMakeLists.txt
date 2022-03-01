## App name
set( ROOT_APP_NAME  test_vad_profile )

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

## Depends on libraries

list( APPEND  DEP_LIBS     
    lib_vad
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
)


## Sources
file( GLOB_RECURSE SOURCES_C  src/*.c )
file( GLOB SOURCES_XC src/*.xc )
file( GLOB SOURCES_SHARED ${SHARED_SRC_PATH}/file_utils/*.c )

file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${XSCOPE_FILEIO_PATH}/src/*.c )
file( GLOB_RECURSE PROFILE_SOURCES ../../../test/lib_adec/test_adec_profile/src/*.c )

unset( SOURCES )
list( APPEND SOURCES ${SOURCES_C} ${SOURCES_SHARED})
unset( SOURCES_XCORE )
list( APPEND SOURCES_XCORE ${SOURCES_XC} ${XSCOPE_FILEIO_SOURCES} ${PROFILE_SOURCES} ${LIB_DSP_SOURCES})
list( APPEND SOURCES ${SOURCES_${CMAKE_SYSTEM_NAME}} )

## Includes
unset( INCLUDES )
list( APPEND  INCLUDES src ${SHARED_SRC_PATH}/file_utils )
unset( INCLUDES_XCORE )
list( APPEND INCLUDES_XCORE ${XSCOPE_FILEIO_PATH} ${XSCOPE_FILEIO_PATH}/api ../../../test/lib_adec/test_adec_profile/src)
list( APPEND INCLUDES ${INCLUDES_${CMAKE_SYSTEM_NAME}} )


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

foreach( PROF_CONFIG VAD_ENABLED VAD_DISABLED )
    set( APP_NAME  ${ROOT_APP_NAME}_${PROF_CONFIG} )

    if ( PROF_CONFIG STREQUAL  VAD_DISABLED)
        list( APPEND  COMPILE_FLAGS -DEMPTY_APP=1 )

    endif()
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
endforeach( PROF_CONFIG VAD_ENABLED VAD_DISABLED )



