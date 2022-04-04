
add_library(avona_module_lib_vnr STATIC)

file(GLOB_RECURSE VNR_SOURCES src/*.c src/*.cc)

target_sources(avona_module_lib_vnr PRIVATE ${VNR_SOURCES})

target_include_directories(avona_module_lib_vnr PUBLIC api src src/model)

set(XCORE_TARGET "XCORE-AI-EXPLORER")
message(STATUS "XCORE_TARGET= ${XCORE_TARGET}")
target_compile_options(avona_module_lib_vnr
    PRIVATE
        "-Os"
        "-g"       
        "-mcmodel=large"
        "-Wno-xcore-fptrgroup"
        "-target=XCORE-AI-EXPLORER"
)

target_compile_definitions(avona_module_lib_vnr
    PUBLIC
        DEBUG_PRINT_ENABLE=1
        TF_LITE_STATIC_MEMORY=1
        __xtflm_conf_h_exists__=1
        )

target_link_libraries(avona_module_lib_vnr
    PUBLIC
        sdk::lib_xs3_math
        sdk::inferencing::lib_tflite_micro
        )

add_library(avona::vnr ALIAS avona_module_lib_vnr)
