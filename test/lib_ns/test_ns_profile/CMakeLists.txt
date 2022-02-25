if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()
#########
## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_test_ns_profile)

file( GLOB_RECURSE SOURCES_C  ../test_wav_ns/src/*.c )
target_sources(avona_test_ns_profile
    PRIVATE
        ${SOURCES_C})

target_include_directories(avona_test_ns_profile
    PRIVATE
        src)

target_link_libraries(avona_test_ns_profile
    PRIVATE
        avona::ns
        avona::example::profile_xcore
        avona::example::fileutils
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_test_ns_profile
        PRIVATE ../test_wav_ns/src/main.xc)

    target_compile_options(avona_test_ns_profile
        PRIVATE
            "-DPROFILE_PROCESSING=1"
            "-target=${XCORE_TARGET}")

    target_link_options(avona_test_ns_profile
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(avona_test_ns_profile
        PRIVATE m)
endif()
