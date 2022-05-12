
## inference
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_library(avona_module_lib_vnr_inference STATIC)

    file(GLOB_RECURSE VNR_INFERENCE_SOURCES src/inference/*.c src/inference/*.cc)

    target_sources(avona_module_lib_vnr_inference PRIVATE ${VNR_INFERENCE_SOURCES})

    target_include_directories(avona_module_lib_vnr_inference PUBLIC api/inference src/inference src/inference/model)

#build/avona_deps/lib_tflite_micro/lib_tflite_micro/src/tflite-xcore-kernels/micro_time.cc needs target to be defined to compile
#/Users/shuchitak/sandboxes/sw_avona_modern_cmake/sw_avona/build/avona_deps/lib_tflite_micro/lib_tflite_micro/src/tflite-xcore-kernels/micro_time.cc:16:37: error: 
#      use of undeclared identifier 'PLATFORM_REFERENCE_HZ'
#int32_t ticks_per_second() { return PLATFORM_REFERENCE_HZ; }

    target_compile_options(avona_module_lib_vnr_inference
        PRIVATE
            "-Os"
            "-g"       
            "-mcmodel=large"
            "-Wno-xcore-fptrgroup"
            "-target=XCORE-AI-EXPLORER"
    )

    target_compile_definitions(avona_module_lib_vnr_inference
        PUBLIC
            DEBUG_PRINT_ENABLE=1
            TF_LITE_STATIC_MEMORY=1
            __xtflm_conf_h_exists__=1
            )

    target_link_libraries(avona_module_lib_vnr_inference
        PUBLIC
            sdk::lib_xs3_math
            sdk::inferencing::lib_tflite_micro
            )

    add_library(avona::vnr::inference ALIAS avona_module_lib_vnr_inference)
endif()

## features
add_library(avona_module_lib_vnr_features STATIC)

file(GLOB_RECURSE VNR_FEATURES_SOURCES src/features/*.c)

target_sources(avona_module_lib_vnr_features PRIVATE ${VNR_FEATURES_SOURCES})

target_include_directories(avona_module_lib_vnr_features PUBLIC api/features src/features)

target_compile_definitions(avona_module_lib_vnr_features PRIVATE HEADROOM_CHECK=0)

target_compile_options(avona_module_lib_vnr_features
    PRIVATE
        "-Os"
        "-g"       
)

target_link_libraries(avona_module_lib_vnr_features
    PUBLIC
        sdk::lib_xs3_math
        )

add_library(avona::vnr::features ALIAS avona_module_lib_vnr_features)
