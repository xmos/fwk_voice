
add_library( delay_buffer  STATIC src/delay_buffer.c )

target_include_directories( delay_buffer PUBLIC src )

target_link_libraries(delay_buffer)

set_target_properties( delay_buffer PROPERTIES   PREFIX      ""
                                            SUFFIX      ".a")
