
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_test_wav_vnr)

target_sources(avona_test_wav_vnr PRIVATE src/test_wav_vnr.c)

target_include_directories(avona_test_wav_vnr PRIVATE src)

target_link_libraries(avona_test_wav_vnr 
    PUBLIC
        avona::vnr::features
        avona_module_lib_vnr_inference_only_for_testing
        avona::example::fileutils)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_test_wav_vnr PRIVATE src/main.xc)

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

    target_compile_options(avona_test_wav_vnr
        PRIVATE ${APP_COMPILER_FLAGS})

    target_link_options(avona_test_wav_vnr
        PRIVATE
            -lquadspi
            -w            
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "C++ Compiler Base Flags" FORCE)

    set(BUILD_FLAGS
      "-Os"
      "-D__xtflm_conf_h_exists__"
      "-DNN_USE_REF"
    )
    target_compile_options(avona_test_wav_vnr
        PRIVATE ${BUILD_FLAGS})

    target_link_options(avona_test_wav_vnr
        PRIVATE
            ${BUILD_FLAGS})

    target_link_libraries(avona_test_wav_vnr PRIVATE "-lpthread")
    target_link_libraries(avona_test_wav_vnr PRIVATE m)
endif()
