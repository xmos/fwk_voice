
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set( CONFIG_XSCOPE_PATH ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/etc )

set(COMMON_SOURCES_C ../../lib_vnr/vnr_unit_tests/feature_extraction/src/main.c)
set(COMMON_SOURCES_XC ../../lib_vnr/vnr_unit_tests/feature_extraction/src/main.xc)

file(GLOB TEST_SOURCES src/test_*.c)
#message(STATUS "${TEST_SOURCES}")
foreach(testfile ${TEST_SOURCES})
    get_filename_component(TESTNAME ${testfile} NAME_WLE)
    #########
    ## Add a build target
    set(APP_NAME fwk_voice_${TESTNAME})
    add_executable(${APP_NAME})

    target_sources(${APP_NAME} PRIVATE ${COMMON_SOURCES_C} ${testfile})

    target_link_libraries(${APP_NAME} 
        PUBLIC
            fwk_voice::ic
            fwk_voice::example::fileutils)

    if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        set(APP_COMPILER_FLAGS
            -Os
            -g
            -target=${XCORE_TARGET}
        )

        target_sources(${APP_NAME} PRIVATE ${COMMON_SOURCES_XC})

        target_compile_options(${APP_NAME}
            PRIVATE ${APP_COMPILER_FLAGS})

        target_link_options(${APP_NAME}
            PRIVATE
                -w            
                "-target=${XCORE_TARGET}"
                "-report"
                "${CONFIG_XSCOPE_PATH}/config.xscope")
    else()
        set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "C++ Compiler Base Flags" FORCE)

        target_link_libraries(${APP_NAME} 
            PUBLIC
                lib_xcore_math)

        target_link_libraries(${APP_NAME} PRIVATE "-lpthread")
        target_link_libraries(${APP_NAME} PRIVATE m)
    endif()
endforeach()
