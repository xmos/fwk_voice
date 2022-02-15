## App name supports inclusion of this whole file from different project
string(COMPARE EQUAL "${APP_NAME}" "" NO_APP_DEFINED)
if (NO_APP_DEFINED)
    set( APP_NAME  test_wav_ic )
endif()

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()



## Depends on libraries
unset( DEP_LIBS_XCORE )
list( APPEND  DEP_LIBS_XCORE  ""  )
list( APPEND  DEP_LIBS_Linux  m   )

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
file( GLOB SOURCES_IC_PROCESS_FRAME  ${SHARED_SRC_PATH}/ic/*.c )
file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${XSCOPE_FILEIO_PATH}/src/*.c )
file( GLOB SOURCES_FILE_UTILS ${SHARED_SRC_PATH}/file_utils/src/*.c ) 

unset( SOURCES )
list( APPEND SOURCES ${SOURCES_C} ${SOURCES_IC_PROCESS_FRAME} ${SOURCES_XC} ${SOURCES_FILE_UTILS})

if ( XCORE )
    list( APPEND SOURCES ${XSCOPE_FILEIO_SOURCES} )
endif()


unset( INCLUDES )
list( APPEND  INCLUDES 
    src 
    ${SHARED_SRC_PATH}/ic 
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
list(APPEND COMPILE_FLAGS  ${ADDITIONAL_COMPILE_FLAGS})
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

add_executable( ${APP_NAME} ${SOURCES} )

target_include_directories( ${APP_NAME} PRIVATE ${INCLUDES})

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
