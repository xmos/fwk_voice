
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#**********************
# Gather Sources
#**********************
file(GLOB_RECURSE APP_SOURCES   src/*.c src/*.xc src/*.cc)
set(APP_INCLUDES src)

#**********************
# Flags
#**********************
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
set(APP_COMPILE_DEFINITIONS
    DEBUG_PRINT_ENABLE=1
    CI_TESTING=${CI_TESTING}
    # NDEBUG                        # define this to remove debug and profiling
    # TF_LITE_STRIP_ERROR_STRINGS   # define this to remove logging
)

set(APP_LINK_OPTIONS
    -lquadspi
    -w
    -report
    -target=${XCORE_TARGET}
    ${CONFIG_XSCOPE_PATH}/config.xscope
)

#**********************
# Tile Targets
#**********************
set(TARGET_NAME avona_example_bare_metal_vnr)
add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS})
target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_libraries(${TARGET_NAME} PUBLIC 
    avona::vnr::inference
    avona::example::fileutils)

target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
unset(TARGET_NAME)
