
set(XSCOPE_FILEIO_PATH ${CMAKE_BINARY_DIR}/fwk_voice_deps)

set(APP_NAME fwk_voice_deps_xscope_fileio) 
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

add_library(fwk_voice_deps::xscope_fileio ALIAS fwk_voice_deps_xscope_fileio)
