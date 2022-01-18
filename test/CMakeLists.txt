#set(DEPS_ROOT ${CMAKE_BINARY_DIR}/deps)  ## For fetching dependencies using cmake
set(DEPS_ROOT ${CMAKE_SOURCE_DIR}/..)

add_subdirectory( shared )

add_subdirectory( lib_meta_data )

add_subdirectory( lib_aec )

add_subdirectory( lib_de )

add_subdirectory( lib_adec )

add_subdirectory( lib_ic )
