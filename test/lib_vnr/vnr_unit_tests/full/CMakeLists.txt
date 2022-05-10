
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)


set(COMMON_SOURCES_C ../feature_extraction/src/main.c)
set(COMMON_SOURCES_XC ../feature_extraction/src/main.xc)

set(APP_COMPILER_FLAGS
    -Os
    -g
    -report
    -fxscope
    -mcmodel=large
    -Wno-xcore-fptrgroup
    ${CMAKE_CURRENT_SOURCE_DIR}/config.xscope
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
    target_include_directories(${APP_NAME} PRIVATE src ${CMAKE_SOURCE_DIR}/modules/lib_vnr/api/features)

    target_link_libraries(${APP_NAME} 
        PUBLIC
            avona::vnr::inference
            avona::vnr::features
            avona::example::fileutils)

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
                "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
    else()
        target_link_libraries(
            PRIVATE m)
    endif()
endforeach()
