
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_example_bare_metal_vnr)

target_sources(avona_example_bare_metal_vnr PRIVATE src/main.c src/vnr_task.c)

target_include_directories(avona_example_bare_metal_vnr PRIVATE src)

set(APP_COMPILER_FLAGS
    -Os
    -g
    -report
    -fxscope
    -mcmodel=large
    -Wno-xcore-fptrgroup    
    -target=${XCORE_TARGET}
)
target_link_libraries(avona_example_bare_metal_vnr 
    PUBLIC
        avona::vnr::features
        avona::vnr::inference
        avona::example::profile_xcore
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_vnr PRIVATE src/main.xc)

    target_compile_options(avona_example_bare_metal_vnr
        PRIVATE ${APP_COMPILER_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/config.xscope)

    target_compile_definitions(avona_example_bare_metal_vnr
        PRIVATE PROFILE_PROCESSING=1)
    
    target_link_options(avona_example_bare_metal_vnr
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

# Run target
add_custom_target(run_avona_example_vnr
  COMMAND xrun --xscope-port localhost:10234 bin/avona_example_bare_metal_vnr.xe
  DEPENDS avona_example_bare_metal_vnr
  COMMENT
    "Run application"
  VERBATIM
)

##########################################################################
# With fileio. Works on M1 Mac
add_executable(avona_example_bare_metal_vnr_fileio)
target_sources(avona_example_bare_metal_vnr_fileio PRIVATE src/with_fileio/main.xc src/with_fileio/main.c src/vnr_task.c)
target_link_libraries(avona_example_bare_metal_vnr_fileio 
    PUBLIC
        avona::vnr::features
        avona::vnr::inference
        avona::example::fileutils
        avona::example::profile_xcore
        )
target_compile_options(avona_example_bare_metal_vnr_fileio
    PRIVATE ${APP_COMPILER_FLAGS} ${CONFIG_XSCOPE_PATH}/config.xscope)

target_compile_definitions(avona_example_bare_metal_vnr_fileio
    PRIVATE PROFILE_PROCESSING=1)

target_link_options(avona_example_bare_metal_vnr_fileio
    PRIVATE
        -lquadspi
        -w            
        "-target=${XCORE_TARGET}"
        "-report"
        "${CONFIG_XSCOPE_PATH}/config.xscope")

