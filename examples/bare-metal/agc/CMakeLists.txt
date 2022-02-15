set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(agc src/agc_app.c)

target_link_libraries(agc lib_agc lib_xs3_math file_utils)

if(XCORE)
  target_sources(agc PRIVATE src/main.xc)

  target_compile_options(agc PRIVATE "-DTEST_WAV_XSCOPE=1"
                                     "-target=${XCORE_TARGET}")

  target_link_options(agc PRIVATE "-target=${XCORE_TARGET}"
                                  "-report"
                                  "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
  set_target_properties(agc PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(agc m)
endif(XCORE)
