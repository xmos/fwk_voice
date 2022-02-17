
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(aec_1_thread src/aec_app.c
                            ${SHARED_SRC_PATH}/aec/aec_process_frame_1thread.c )

target_link_libraries(aec_1_thread lib_aec sdk::lib_xs3_math file_utils)

target_include_directories(aec_1_thread PRIVATE ${SHARED_SRC_PATH}/aec)

if(XCORE)
  target_sources(aec_1_thread PRIVATE src/main.xc)

  target_compile_options(aec_1_thread PRIVATE "-DTEST_WAV_XSCOPE=1"
                                              "-target=${XCORE_TARGET}")

  target_link_options(aec_1_thread PRIVATE "-target=${XCORE_TARGET}"
                                  "-report"
                                  "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
  set_target_properties(aec_1_thread PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(aec_1_thread m)
endif()
