set( LIB_NAME   lib_noise_suppression  )

add_library( ${LIB_NAME} STATIC src/sup_impl.c src/sup_noise.c )

target_include_directories( ${LIB_NAME} PUBLIC api src )

target_link_libraries( ${LIB_NAME} lib_xs3_math )

set_target_properties( ${LIB_NAME} PROPERTIES   PREFIX      ""
                                                SUFFIX      ".a"        )
                                                