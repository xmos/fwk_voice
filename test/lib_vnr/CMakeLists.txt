
# Compile VNR inference as a static library only for speeding up tests compilation. Note that it's compiled with target=XCORE-AI-EXPLORER, so will
# only link against applications targeting XCORE-AI-EXPLORER which is fine for testing. lib_tflite_micro compilation is dependant on the HW
# target which is why we don't build avona::vnr::inference as a static library in the modules/lib_vnr CMake file.

add_library(avona_module_lib_vnr_inference_only_for_testing STATIC)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(avona_module_lib_vnr_inference_only_for_testing PRIVATE "-target=XCORE-AI-EXPLORER")
else()
    target_compile_options(avona_module_lib_vnr_inference_only_for_testing PRIVATE "-fPIC")
endif()
set(VNR_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules/lib_vnr")
message(STATUS "VNR_MODULE_PATH = ${VNR_MODULE_PATH}") 
target_include_directories(avona_module_lib_vnr_inference_only_for_testing PUBLIC ${VNR_MODULE_PATH}/api/common ${VNR_MODULE_PATH}/api/inference)
target_link_libraries(avona_module_lib_vnr_inference_only_for_testing PRIVATE avona::vnr::inference)  
target_link_libraries(avona_module_lib_vnr_inference_only_for_testing PUBLIC core::xs3_math) 

add_subdirectory(vnr_unit_tests)

add_subdirectory(test_wav_vnr)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory(profile_memory)
endif()
