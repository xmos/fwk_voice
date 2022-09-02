
# Compile VNR inference as a static library only for speeding up tests compilation. Note that it's compiled with target=XCORE-AI-EXPLORER, so will
# only link against applications targeting XCORE-AI-EXPLORER which is fine for testing. lib_tflite_micro compilation is dependant on the HW
# target which is why we don't build fwk_voice::vnr::inference as a static library in the modules/lib_vnr CMake file.

#add_library(fwk_voice_calc_vnr_pred_only_for_testing STATIC)
#if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
#    target_compile_options(fwk_voice_calc_vnr_pred_only_for_testing PRIVATE "-target=XCORE-AI-EXPLORER")
#else()
#    target_compile_options(fwk_voice_calc_vnr_pred_only_for_testing PRIVATE "-fPIC")
#endif()
#set(CALC_VNR_PRED_PATH "${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/calc_vnr_pred/")
#target_include_directories(fwk_voice_calc_vnr_pred_only_for_testing PUBLIC ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src/calc_vnr_pred/src)
#target_link_libraries(fwk_voice_calc_vnr_pred_only_for_testing PUBLIC fwk_voice::shared::calc_vnr_pred)

add_subdirectory(vnr_unit_tests)

add_subdirectory(test_wav_vnr)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory(profile_memory)
endif()
