add_library(lib_agc STATIC src/agc_impl.c)

target_include_directories(lib_agc PUBLIC api)

target_link_libraries(lib_agc PUBLIC sdk::lib_xs3_math)

set_target_properties(lib_agc PROPERTIES PREFIX ""
                                         SUFFIX ".a")
