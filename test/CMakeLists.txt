#set(DEPS_ROOT ${CMAKE_BINARY_DIR}/deps)  ## For fetching dependencies using cmake
set(DEPS_ROOT ${CMAKE_SOURCE_DIR}/..)
set( SHARED_SRC_PATH ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src )
set( XSCOPE_FILEIO_PATH ${SHARED_SRC_PATH}/xscope_fileio/xscope_fileio )

add_subdirectory( shared )

add_subdirectory( lib_ns )

add_subdirectory( lib_aec )

add_subdirectory( lib_adec )

add_subdirectory( lib_ic )

add_subdirectory( lib_agc )
