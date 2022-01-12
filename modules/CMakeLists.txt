set(LIB_XS3_MATH_PATH ${CMAKE_SOURCE_DIR}/../lib_xs3_math/lib_xs3_math)
add_subdirectory( ${LIB_XS3_MATH_PATH} ${CMAKE_CURRENT_BINARY_DIR}/libs/)

add_subdirectory( lib_meta_data )

add_subdirectory( lib_aec )

add_subdirectory( lib_agc )

add_subdirectory( lib_de )

add_subdirectory( lib_adec )
