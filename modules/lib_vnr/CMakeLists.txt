
add_library(avona_module_lib_vnr_inference STATIC)

file(GLOB_RECURSE VNR_SOURCES src/inference/*.c src/inference/*.cc)

target_sources(avona_module_lib_vnr_inference PRIVATE ${VNR_SOURCES})

target_include_directories(avona_module_lib_vnr_inference PUBLIC api/inference src/inference src/inference/model)

set(XCORE_TARGET "XCORE-AI-EXPLORER")
message(STATUS "XCORE_TARGET= ${XCORE_TARGET}")
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
