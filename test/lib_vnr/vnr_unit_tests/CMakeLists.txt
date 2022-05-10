
set( CONFIG_XSCOPE_PATH ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/etc )

add_subdirectory(feature_extraction)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory(inference)
    add_subdirectory(full)
endif()
