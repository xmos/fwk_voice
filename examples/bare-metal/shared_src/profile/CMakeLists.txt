
add_library( profile  STATIC src/profile.c )

target_include_directories( profile PUBLIC src )

target_link_libraries(profile)

set_target_properties( profile PROPERTIES   PREFIX      ""
                                            SUFFIX      ".a")
