# Get path of xmos ai tools
set(CMD "\
import os; \
import xmos_ai_tools.runtime as rt; \
print(os.path.dirname(rt.__file__)) \
")

execute_process(
    COMMAND python -c "${CMD}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE XMOS_AITOOLSLIB_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add tflite_micro 
set(XMOS_AITOOLSLIB_PATH_CMAKE "${XMOS_AITOOLSLIB_PATH}/buildfiles/aitoolslib.cmake")

if(XMOS_AITOOLSLIB_PATH STREQUAL "")
    message(FATAL_ERROR "Path to XMOS AI tools NOT found")
elseif(NOT EXISTS  ${XMOS_AITOOLSLIB_PATH_CMAKE})
    message(FATAL_ERROR "Cmake file 'aitoolslib.cmake' NOT found in this path")
else()
    message(STATUS "Found python package xmos-ai-tools at: ${XMOS_AITOOLSLIB_PATH}")
    include(${XMOS_AITOOLSLIB_PATH_CMAKE})
    set(LIB_NAME tflite_micro)
    add_library(${LIB_NAME} STATIC IMPORTED GLOBAL)
    target_compile_definitions(${LIB_NAME} INTERFACE ${XMOS_AITOOLSLIB_DEFINITIONS})
    set_target_properties(${LIB_NAME}  PROPERTIES
        LINKER_LANGUAGE CXX
        IMPORTED_LOCATION ${XMOS_AITOOLSLIB_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${XMOS_AITOOLSLIB_INCLUDES})
endif()

## Export model
set(MODEL_IN_PATH python/model/model_output/trained_model.tflite)
set(MODEL_OUT_PATH src/inference/model/trained_model_xcore.tflite)
set(MODEL_N_CORES 1)
set(MODEL_TH 0.50)

execute_process(
    COMMAND xcore-opt ${MODEL_IN_PATH} -tc ${MODEL_N_CORES} -o ${MODEL_OUT_PATH} --xcore-conv-err-threshold ${MODEL_TH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

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
	tflite_micro
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
