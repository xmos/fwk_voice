
## inference
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
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
