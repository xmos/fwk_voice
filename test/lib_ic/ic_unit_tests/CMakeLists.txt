## App name
set( APP_NAME ic_unit_tests )

# Auto-generate schedule and top level config files

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

#copy conftest.py in the build directory since pytest_collect_file only collects tests from the directory tree where conftest.py is present
configure_file( conftest.py conftest.py COPYONLY ) 
set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen )

message(STATUS "${APP_NAME} ic build config:  ${IC_UNIT_TESTS_BUILD_CONFIG}" )
if (NOT DEFINED TESTS_SPEEDUP_FACTOR)
    set (TESTS_SPEEDUP_FACTOR 1)
endif()
message( STATUS "Unit tests speedup factor ${TESTS_SPEEDUP_FACTOR}" )

file(MAKE_DIRECTORY ${AUTOGEN_DIR})

## Depends on libraries
list( APPEND  DEP_LIBS_XCORE  ""  )

list( APPEND  DEP_LIBS        
    lib_xs3_math
    lib_ic
    test_shared
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
list( APPEND  COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} -DSPEEDUP_FACTOR=${TESTS_SPEEDUP_FACTOR} )

## Sources
# Unity
set( UNITY_PATH ${DEPS_ROOT}/Unity/src )
file( GLOB UNITY_SOURCES ${UNITY_PATH}/*.c )

# All common sources
list( APPEND  SOURCES ${AUTOGEN_SOURCES} ${UNITY_SOURCES} )

## Includes
list( APPEND  INCLUDES src ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/pseudo_rand ${AUTOGEN_DIR} ${UNITY_PATH} )
list( APPEND  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/lib_ic/src)

## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Set unity runner generate script
set( GEN_RUNNER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/generate_unity_runner.py )

# Create directory for runner files
set( RUNNERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.runners )
file( MAKE_DIRECTORY ${RUNNERS_DIR} )

file( GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c )

# For every source file in ic_unit_tests/src
foreach( testfile ${TEST_SOURCES} )
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
        COMMENT "Generating IC runner" )
    
    if (${testfile} MATCHES "test_adaption_controller.c")
        set (SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/lib_aec/src/aec_priv_impl.c)
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

