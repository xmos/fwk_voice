
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
add_executable(aec_2_threads ${SOURCES_APP} 
                            ${SHARED_SRC_PATH}/aec/aec_process_frame_2threads.c )

target_link_libraries(aec_2_threads lib_aec lib_xs3_math file_utils)

target_include_directories(aec_2_threads PRIVATE src ${SHARED_SRC_PATH}/aec)

if(XCORE)
  target_sources(aec_2_threads PRIVATE src/main.xc)

  target_compile_options(aec_2_threads PRIVATE "-DTEST_WAV_XSCOPE=1"
                                              "-target=${XCORE_TARGET}")

  target_link_options(aec_2_threads PRIVATE "-target=${XCORE_TARGET}"
                                  "-report"
                                  "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
  set_target_properties(aec_2_threads PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(aec_2_threads m)
endif()
