
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_example_bare_metal_vnr)

target_sources(avona_example_bare_metal_vnr PRIVATE src/main.c)

target_include_directories(avona_example_bare_metal_vnr PRIVATE src)

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
target_link_libraries(avona_example_bare_metal_vnr 
    PUBLIC
        avona::vnr::features
        avona::vnr::inference
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_vnr PRIVATE src/main.xc)

    target_compile_options(avona_example_bare_metal_vnr
        PRIVATE ${APP_COMPILER_FLAGS})

    target_link_options(avona_example_bare_metal_vnr
        PRIVATE
            -lquadspi
            -w            
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
else()
    target_link_libraries(
        PRIVATE m)
endif()
