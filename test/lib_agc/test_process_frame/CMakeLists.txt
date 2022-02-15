#copy conftest.py in the build directory since pytest_collect_file only collects tests from the directory tree where conftest.py is present
configure_file( conftest.py conftest.py COPYONLY )

## Sources
# Unity
file(GLOB UNITY_SOURCES ${UNITY_PATH}/src/*.c)

## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Set unity runner generate script
set(GEN_RUNNER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/generate_unity_runner.py)

# Create directory for runner files
set(RUNNERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.runners)
file(MAKE_DIRECTORY ${RUNNERS_DIR})

file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# For every source file in aec_unit_tests/src
foreach(testfile ${TEST_SOURCES})
    get_filename_component(TESTNAME ${testfile} NAME_WLE)

    # Create runner file directory
    file(MAKE_DIRECTORY ${RUNNERS_DIR}/${TESTNAME})

    #########
    ## Create runner file
    set(RUNNER_FILE ${RUNNERS_DIR}/${TESTNAME}/${TESTNAME}_Runner.c)
    set(GEN_RUNNER_SCRIPT_BYPRODUCTS ${RUNNER_FILE})

    unset(GEN_RUNNER_SCRIPT_ARGS)
    list(APPEND GEN_RUNNER_SCRIPT_ARGS --project-root ${UNITY_PATH}/..)
    list(APPEND GEN_RUNNER_SCRIPT_ARGS --source-file ${testfile} )
    list(APPEND GEN_RUNNER_SCRIPT_ARGS --runner-file ${RUNNER_FILE})

    ## Add command to generate runner file
    add_custom_command(
        OUTPUT ${RUNNER_FILE}
        COMMAND ${Python3_EXECUTABLE} ${GEN_RUNNER_SCRIPT} ${GEN_RUNNER_SCRIPT_ARGS}
        COMMENT "Generating AGC runner")

    #########
    ## Add a build target
    add_executable(${TESTNAME} ${testfile} ${RUNNER_FILE} ${UNITY_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/pseudo_rand/pseudo_rand.c)

    target_include_directories(${TESTNAME} PRIVATE src
                                                   ${UNITY_PATH}/src
                                                   ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/pseudo_rand)

    target_link_libraries(${TESTNAME} lib_agc lib_xs3_math)

    if(XCORE)
        target_compile_options(${TESTNAME} PRIVATE "-DUNITY_SUPPORT_64"
                                                   "-Wno-xcore-fptrgroup"
                                                   "-report"
                                                   "-DSPEEDUP_FACTOR=${TEST_SPEEDUP_FACTOR}")
        target_link_options(${TESTNAME} PRIVATE "-target=${XCORE_TARGET}")
        set_target_properties(${TESTNAME} PROPERTIES SUFFIX ".xe")
    else()
        target_link_libraries(${TESTNAME} m)
    endif(XCORE)
endforeach(testfile ${TEST_SOURCES})
