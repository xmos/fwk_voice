set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(agc src/agc_app.c
                   ${SHARED_SRC_PATH}/file_utils/fileio.c
                   ${SHARED_SRC_PATH}/file_utils/wav_utils.c)

target_include_directories(agc PRIVATE ${SHARED_SRC_PATH}/file_utils)

target_link_libraries(agc lib_agc lib_xs3_math)

if(XCORE)
  target_sources(agc PRIVATE src/main.xc
                             ${XSCOPE_FILEIO_PATH}/src/xscope_io_device.c)

  target_include_directories(agc PRIVATE ${XSCOPE_FILEIO_PATH}
                                         ${XSCOPE_FILEIO_PATH}/api)

  target_compile_options(agc PRIVATE "-DTEST_WAV_XSCOPE=1"
                                     "-target=${XCORE_TARGET}")

  target_link_options(agc PRIVATE "-target=${XCORE_TARGET}"
                                  "-report"
                                  "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
  set_target_properties(agc PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(agc m)
endif(XCORE)
