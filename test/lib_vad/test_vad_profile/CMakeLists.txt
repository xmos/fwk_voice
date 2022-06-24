## App name
set( ROOT_APP_NAME  fwk_voice_test_vad_profile )

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

#########
## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

foreach( PROF_CONFIG VAD_ENABLED VAD_DISABLED )
    set( APP_NAME  ${ROOT_APP_NAME}_${PROF_CONFIG} )

    add_executable(${APP_NAME})

    file( GLOB_RECURSE SOURCES_C  src/*.c )
    target_sources(${APP_NAME}
        PRIVATE
            ${SOURCES_C}
            src/main.xc)

    target_include_directories(${APP_NAME}
        PRIVATE
            src)

    target_link_libraries(${APP_NAME}
        PUBLIC
            fwk_voice::vad
            fwk_voice::example::fileutils
            fwk_voice::example::profile_xcore
            )

    target_compile_definitions(${APP_NAME}
        PRIVATE
            PROFILE_PROCESSING=1)

    if ( PROF_CONFIG STREQUAL  VAD_DISABLED)
        target_compile_definitions(${APP_NAME}
            PRIVATE
                EMPTY_APP=1)
    endif()

    target_compile_options(${APP_NAME}
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(${APP_NAME}
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
endforeach()



