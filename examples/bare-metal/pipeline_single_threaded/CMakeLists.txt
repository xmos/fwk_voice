
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
file( GLOB SOURCES_SHARED_AEC ${SHARED_SRC_PATH}/aec/aec_process_frame_1thread.c )
file( GLOB SOURCES_PIPELINE_STAGE_1  ${SHARED_SRC_PATH}/pipeline_stage_1/*.c )

add_executable(pipeline_single_threaded ${SOURCES_APP} 
                                        ${SOURCES_SHARED_AEC} 
                                        ${SOURCES_PIPELINE_STAGE_1})

target_link_libraries(pipeline_single_threaded lib_aec
                                               lib_adec
                                               lib_ns
                                               lib_agc
                                               lib_xs3_math
                                               file_utils
                                               delay_buffer)

target_include_directories(pipeline_single_threaded PRIVATE src
                                                            ${SHARED_SRC_PATH}/aec
                                                            ${SHARED_SRC_PATH}/pipeline_stage_1)

if(XCORE)
  target_sources(pipeline_single_threaded PRIVATE src/main.xc)

  target_compile_options(pipeline_single_threaded PRIVATE "-DTEST_WAV_XSCOPE=1"
                                                          "-DNUM_AEC_THREADS=1"
                                                          "-target=${XCORE_TARGET}")

  target_link_options(pipeline_single_threaded PRIVATE "-target=${XCORE_TARGET}"
                                                       "-report"
                                                       "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")

  set_target_properties(pipeline_single_threaded PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(pipeline_single_threaded m)
endif()

