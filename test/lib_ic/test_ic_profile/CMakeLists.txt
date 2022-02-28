## App name
set( APP_NAME  test_ic_profile )

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#######
add_executable(avona_test_ic_profile)

target_link_libraries(avona_test_ic_profile
    PUBLIC
        avona::example::test_wav_ic
        )

target_compile_definitions(avona_test_ic_profile
    PRIVATE
        PROFILE_PROCESSING=1)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(avona_test_ic_profile
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_test_ic_profile
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(avona_test_ic_profile
        PRIVATE m)
endif()
