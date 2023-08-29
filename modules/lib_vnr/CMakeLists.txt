
## VNR Inference
add_library(fwk_voice_module_lib_vnr_inference STATIC)

file(GLOB_RECURSE VNR_INFERENCE_SOURCES src/inference/model/*.cpp src/inference/*.cc src/inference/*.cpp)

target_sources(fwk_voice_module_lib_vnr_inference PRIVATE ${VNR_INFERENCE_SOURCES})

target_include_directories(fwk_voice_module_lib_vnr_inference PUBLIC api/common api/inference)

target_link_libraries(fwk_voice_module_lib_vnr_inference
    PUBLIC
        lib_xcore_math
        )

target_link_libraries(fwk_voice_module_lib_vnr_inference
    PRIVATE
	sdk::inferencing::lib_tflite_micro
	)

add_library(fwk_voice::vnr::inference ALIAS fwk_voice_module_lib_vnr_inference)

## VNR Feature extraction
add_library(fwk_voice_module_lib_vnr_features STATIC)

file(GLOB_RECURSE VNR_FEATURES_SOURCES src/features/*.c)

target_sources(fwk_voice_module_lib_vnr_features PRIVATE ${VNR_FEATURES_SOURCES})

target_include_directories(fwk_voice_module_lib_vnr_features PUBLIC api/features api/common)

target_compile_definitions(fwk_voice_module_lib_vnr_features PRIVATE HEADROOM_CHECK=0)

target_compile_options(fwk_voice_module_lib_vnr_features
    PRIVATE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_lib_vnr_features
    PUBLIC
        lib_xcore_math
        )

add_library(fwk_voice::vnr::features ALIAS fwk_voice_module_lib_vnr_features)
