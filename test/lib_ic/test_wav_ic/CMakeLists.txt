## App name supports inclusion of this whole file from different project
string(COMPARE EQUAL "${APP_NAME}" "" NO_APP_DEFINED)
if (NO_APP_DEFINED)
    set( APP_NAME  test_wav_ic )
endif()

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#######
add_executable(avona_${APP_NAME})

target_link_libraries(avona_${APP_NAME}
    PUBLIC
        avona::example::test_wav_ic)

target_compile_definitions(avona_${APP_NAME}
    PRIVATE
        ${ADDITIONAL_COMPILE_FLAGS})

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(avona_${APP_NAME}
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_${APP_NAME}
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(avona_test_wav_ic
        PRIVATE m)
endif()
