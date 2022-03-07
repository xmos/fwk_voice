
set(XSCOPE_FILEIO_PATH ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src)

set(APP_NAME avona_deps_xscope_fileio) 
add_library(${APP_NAME}  INTERFACE)
        
target_sources(${APP_NAME}
    INTERFACE
        ${XSCOPE_FILEIO_PATH}/xscope_fileio/xscope_fileio/src/xscope_io_device.c)

target_include_directories(${APP_NAME}
    INTERFACE
        ${XSCOPE_FILEIO_PATH}/xscope_fileio/xscope_fileio
        ${XSCOPE_FILEIO_PATH}/xscope_fileio/xscope_fileio/api)

target_compile_definitions(${APP_NAME}
    INTERFACE
        TEST_WAV_XSCOPE=1)

add_library(avona_deps::xscope_fileio ALIAS avona_deps_xscope_fileio)
