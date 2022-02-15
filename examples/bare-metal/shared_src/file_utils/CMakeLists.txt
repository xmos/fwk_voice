
file( GLOB_RECURSE    FILE_UTILS_SOURCES       src/*.c )

if( XCORE )
    list( APPEND FILE_UTILS_SOURCES ${XSCOPE_FILEIO_PATH}/src/xscope_io_device.c)
endif()

add_library( file_utils  STATIC  ${FILE_UTILS_SOURCES} )

target_include_directories( file_utils PUBLIC src ${XSCOPE_FILEIO_PATH} ${XSCOPE_FILEIO_PATH}/api)

target_link_libraries(file_utils)

if( XCORE )
    target_compile_options( file_utils PRIVATE -DTEST_WAV_XSCOPE)
endif()

set_target_properties( file_utils PROPERTIES   PREFIX      ""
                                            SUFFIX      ".a")
