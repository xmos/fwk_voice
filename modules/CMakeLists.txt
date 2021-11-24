set(LIB_XS3_MATH_PATH ${XCORE_SDK_PATH}/modules/lib_xs3_math/lib_xs3_math)

add_subdirectory( ${LIB_XS3_MATH_PATH} ${CMAKE_CURRENT_BINARY_DIR}/libs/)

add_subdirectory( lib_meta_data )

add_subdirectory( lib_aec )
