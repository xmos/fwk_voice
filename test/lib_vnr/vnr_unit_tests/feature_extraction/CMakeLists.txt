
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)


set(COMMON_SOURCES_C src/main.c)
set(COMMON_SOURCES_XC src/main.xc)

set(APP_COMPILER_FLAGS
    -Os
    -g
    -report
    -fxscope
    -mcmodel=large
    -Wno-xcore-fptrgroup
    ${CONFIG_XSCOPE_PATH}/config.xscope
    -target=${XCORE_TARGET}
)

file(GLOB TEST_SOURCES src/test_*.c)
message(STATUS "${TEST_SOURCES}")
foreach(testfile ${TEST_SOURCES})
    get_filename_component(TESTNAME ${testfile} NAME_WLE)
    #########
    ## Add a build target
    set(APP_NAME avona_${TESTNAME})
    add_executable(${APP_NAME})

    target_sources(${APP_NAME} PRIVATE ${COMMON_SOURCES_C} ${testfile})
    target_include_directories(${APP_NAME} PRIVATE src)
    
    # For X86 build, link to math library before linking the modules since linking order seems to matter on some gcc versions.
    # If math is not linked before lib_xs3_math, xs3_scalar_float_s32.c:(.text+0xe4): undefined reference to `roundf' error is occasionally seen
    # When compiling for x86 on the Jenkins xcore.ai agent.
    if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        target_link_libraries(${APP_NAME} PRIVATE m)
    endif()

    target_link_libraries(${APP_NAME} 
        PUBLIC
            avona::vnr::features
            avona::example::fileutils)

    if (${testfile} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}/src/test_vnr_extract_features.c )
        target_link_libraries(${APP_NAME} 
            PUBLIC
                avona::vnr::inference # test_vnr_extract_features also compares quantised output as an extra step so need to link inference
                )
    endif()

    if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        target_sources(${APP_NAME} PRIVATE ${COMMON_SOURCES_XC})

        target_compile_options(${APP_NAME}
            PRIVATE ${APP_COMPILER_FLAGS})

        target_link_options(${APP_NAME}
            PRIVATE
                -lquadspi
                -w            
                "-target=${XCORE_TARGET}"
                "-report"
                "${CONFIG_XSCOPE_PATH}/config.xscope")
    else()
        set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "C++ Compiler Base Flags" FORCE)

        set(BUILD_FLAGS
          "-Os"
          "-D__xtflm_conf_h_exists__"
          "-DNN_USE_REF"
        )
        target_compile_options(${APP_NAME}
            PRIVATE ${BUILD_FLAGS})

        target_link_options(${APP_NAME}
            PRIVATE
                ${BUILD_FLAGS})

        target_link_libraries(${APP_NAME} 
            PUBLIC
                sdk::lib_xs3_math)

        target_link_libraries(${APP_NAME} PRIVATE "-lpthread")
    endif()
endforeach()
