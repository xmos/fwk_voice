
## inference
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    #  lib_tflite_micro compilation is dependant on the HW target which is why we don't build avona::vnr::inference as a static library
    add_library(avona_module_lib_vnr_inference INTERFACE)

    file(GLOB_RECURSE VNR_INFERENCE_SOURCES src/inference/*.c src/inference/*.cc)

    target_sources(avona_module_lib_vnr_inference INTERFACE ${VNR_INFERENCE_SOURCES})

    target_include_directories(avona_module_lib_vnr_inference INTERFACE api/inference src/inference src/inference/model)

    target_compile_options(avona_module_lib_vnr_inference
        INTERFACE
            "-Os"
            "-mcmodel=large"
            "-Wno-xcore-fptrgroup"
    )

    target_compile_definitions(avona_module_lib_vnr_inference
        INTERFACE
            TF_LITE_STATIC_MEMORY=1
            TF_LITE_STRIP_ERROR_STRINGS=1            
            __xtflm_conf_h_exists__=1
            )

    target_link_libraries(avona_module_lib_vnr_inference
        INTERFACE
            core::xs3_math
            )
    target_link_libraries(avona_module_lib_vnr_inference
        INTERFACE
            sdk::inferencing::lib_tflite_micro
            )

    add_library(avona::vnr::inference ALIAS avona_module_lib_vnr_inference)

    # Compile as a static library only for speeding up tests compilation. Note that it's compiled with target=XCORE-AI-EXPLORER, so will
    # only link against applications targeting XCORE-AI-EXPLORER which is fine for testing. lib_tflite_micro compilation dependant on the HW
    # target which is why we don't build avona::vnr::inference as a static library
    add_library(avona_module_lib_vnr_inference_only_for_testing STATIC)
    target_compile_options(avona_module_lib_vnr_inference_only_for_testing PRIVATE "-target=XCORE-AI-EXPLORER")
    target_include_directories(avona_module_lib_vnr_inference_only_for_testing PUBLIC api/inference src/inference src/inference/model)
    target_link_libraries(avona_module_lib_vnr_inference_only_for_testing PRIVATE avona::vnr::inference)  
    target_link_libraries(avona_module_lib_vnr_inference_only_for_testing PUBLIC core::xs3_math) 
else()
    add_library(avona_module_lib_vnr_inference INTERFACE)

    set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "C++ Compiler Base Flags" FORCE)

    set(BUILD_FLAGS
      "-Os"
      "-D__xtflm_conf_h_exists__"
      "-DNN_USE_REF" 
    )
    
    file(GLOB_RECURSE VNR_INFERENCE_SOURCES src/inference/*.c src/inference/*.cc)

    target_sources(avona_module_lib_vnr_inference INTERFACE ${VNR_INFERENCE_SOURCES})

    target_include_directories(avona_module_lib_vnr_inference INTERFACE api/inference src/inference src/inference/model)

    target_compile_options(avona_module_lib_vnr_inference
        INTERFACE
            ${BUILD_FLAGS}
    )
    target_link_options(avona_module_lib_vnr_inference INTERFACE ${BUILD_FLAGS})
    
    target_link_libraries(avona_module_lib_vnr_inference
        INTERFACE
            core::xs3_math
            )

    target_link_libraries(avona_module_lib_vnr_inference
        INTERFACE
            sdk::inferencing::lib_tflite_micro
            )

    add_library(avona::vnr::inference ALIAS avona_module_lib_vnr_inference)
    
    # Compile as a static library only for speeding up tests compilation
    add_library(avona_module_lib_vnr_inference_only_for_testing STATIC)
    target_include_directories(avona_module_lib_vnr_inference_only_for_testing PUBLIC api/inference src/inference src/inference/model)
    target_link_libraries(avona_module_lib_vnr_inference_only_for_testing PRIVATE avona::vnr::inference)  
    target_link_libraries(avona_module_lib_vnr_inference_only_for_testing PUBLIC core::xs3_math) 
endif()

## features
add_library(avona_module_lib_vnr_features STATIC)

file(GLOB_RECURSE VNR_FEATURES_SOURCES src/features/*.c)

target_sources(avona_module_lib_vnr_features PRIVATE ${VNR_FEATURES_SOURCES})

target_include_directories(avona_module_lib_vnr_features PUBLIC api/features)

target_compile_definitions(avona_module_lib_vnr_features PRIVATE HEADROOM_CHECK=0)

target_compile_options(avona_module_lib_vnr_features
    PRIVATE
        "-Os"
        "-g"       
)

target_link_libraries(avona_module_lib_vnr_features
    PUBLIC
        core::xs3_math
        )

add_library(avona::vnr::features ALIAS avona_module_lib_vnr_features)
