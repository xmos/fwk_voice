
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
add_executable(ic_example ${SOURCES_APP}) 

target_link_libraries(ic_example lib_ic lib_xs3_math file_utils)

target_include_directories(ic_example PRIVATE src)

if(XCORE)
  target_sources(ic_example PRIVATE src/main.xc)

  target_include_directories(ic_example PRIVATE ../../../test/lib_adec/test_adec_profile/src)

  target_compile_options(ic_example PRIVATE "-DTEST_WAV_XSCOPE=1"
                                              "-target=${XCORE_TARGET}")

  target_link_options(ic_example PRIVATE "-target=${XCORE_TARGET}"
                                  "-report"
                                  "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
  set_target_properties(ic_example PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(ic_example m)
endif()
