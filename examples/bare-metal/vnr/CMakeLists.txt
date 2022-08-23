
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

############# VNR example that has xscope host running on python. Doesn't work on M1 Mac and does not compile for x86
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)

    add_executable(fwk_voice_example_bare_metal_vnr)

    target_sources(fwk_voice_example_bare_metal_vnr PRIVATE src/main.c src/vnr_task.c)

    target_include_directories(fwk_voice_example_bare_metal_vnr PRIVATE src)

    set(APP_COMPILER_FLAGS
        -Os
        -g
        -report
        -fxscope
        -mcmodel=large
        -Wno-xcore-fptrgroup    
        -target=${XCORE_TARGET}
    )
    target_link_libraries(fwk_voice_example_bare_metal_vnr 
        PUBLIC
            fwk_voice::vnr::features
            fwk_voice::vnr::inference
            fwk_voice::example::profile_xcore
            )

    target_sources(fwk_voice_example_bare_metal_vnr PRIVATE src/main.xc)

    target_compile_options(fwk_voice_example_bare_metal_vnr
        PRIVATE ${APP_COMPILER_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/config.xscope)

    target_compile_definitions(fwk_voice_example_bare_metal_vnr
        PRIVATE PROFILE_PROCESSING=1)
    
    target_link_options(fwk_voice_example_bare_metal_vnr
        PRIVATE
            -w            
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
endif()

##########################################################################

############# VNR example that uses xscope_fileio to run python host. Works on M1 Mac and compiles for both xcore device and x86
add_executable(fwk_voice_example_bare_metal_vnr_fileio)

target_sources(fwk_voice_example_bare_metal_vnr_fileio PRIVATE src/with_fileio/vnr_task.c)

target_link_libraries(fwk_voice_example_bare_metal_vnr_fileio 
    PUBLIC
        fwk_voice::vnr::features
        fwk_voice::vnr::inference
        fwk_voice::example::fileutils
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_bare_metal_vnr_fileio PRIVATE src/with_fileio/main.xc)
    target_link_libraries(fwk_voice_example_bare_metal_vnr_fileio 
        PUBLIC
            fwk_voice::example::profile_xcore
            )
    target_compile_options(fwk_voice_example_bare_metal_vnr_fileio
        PRIVATE ${APP_COMPILER_FLAGS} ${CONFIG_XSCOPE_PATH}/config.xscope)

    target_compile_definitions(fwk_voice_example_bare_metal_vnr_fileio
        PRIVATE PROFILE_PROCESSING=1)

    target_link_options(fwk_voice_example_bare_metal_vnr_fileio
        PRIVATE
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
    target_compile_options(fwk_voice_example_bare_metal_vnr_fileio
        PRIVATE ${BUILD_FLAGS})

    target_link_options(fwk_voice_example_bare_metal_vnr_fileio
        PRIVATE
            ${BUILD_FLAGS})

    target_link_libraries(fwk_voice_example_bare_metal_vnr_fileio PRIVATE "-lpthread")
    target_link_libraries(fwk_voice_example_bare_metal_vnr_fileio PRIVATE m)

endif()

