set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#add_executable(agc_example)

add_executable(agc_example src/agc_app.c
                           ../shared_src/file_utils/fileio.c
                           ../shared_src/file_utils/wav_utils.c)

target_include_directories(agc_example PRIVATE ../shared_src/file_utils)

target_link_libraries(agc_example lib_agc lib_xs3_math)

if(XCORE)
  target_sources(agc_example PRIVATE src/main.xc
                                     ${DEPS_ROOT}/xscope_fileio/xscope_fileio/src/xscope_io_device.c)

  target_include_directories(agc_example PRIVATE ${DEPS_ROOT}/xscope_fileio/xscope_fileio
                                                 ${DEPS_ROOT}/xscope_fileio/xscope_fileio/api)

  target_compile_options(agc_example PRIVATE "-DTEST_WAV_XSCOPE=1"
                                             "-target=${XCORE_TARGET}")

  target_link_options(agc_example PRIVATE "-target=${XCORE_TARGET}"
                                          "-report"
                                          "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
  set_target_properties(agc_example PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(agc_example m)
endif(XCORE)
