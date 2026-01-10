
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
    set(TFLM_LIB_NAME tflite_micro)
    add_library(${TFLM_LIB_NAME} STATIC IMPORTED GLOBAL)
    target_compile_definitions(${TFLM_LIB_NAME} INTERFACE ${XMOS_AITOOLSLIB_DEFINITIONS})
    set_target_properties(${TFLM_LIB_NAME}  PROPERTIES
        LINKER_LANGUAGE CXX
        IMPORTED_LOCATION ${XMOS_AITOOLSLIB_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${XMOS_AITOOLSLIB_INCLUDES})
endif()

## Export model
set(MODEL_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen/vnr_model/)
set(MODEL_IN_PATH ${CMAKE_CURRENT_LIST_DIR}/python/model/trained_model.tflite)
set(MODEL_OUT_PATH ${MODEL_OUT_DIR}/trained_model_xcore.tflite)
set(MODEL_N_CORES 1)
set(MODEL_TH 0.50)

file(MAKE_DIRECTORY ${MODEL_OUT_DIR})

add_custom_command(
    OUTPUT ${MODEL_OUT_PATH}.cpp ${MODEL_OUT_PATH}.h ${MODEL_OUT_PATH}
    COMMAND xcore-opt ${MODEL_IN_PATH} -tc ${MODEL_N_CORES} -o ${MODEL_OUT_PATH} --xcore-conv-err-threshold ${MODEL_TH}
    DEPENDS ${MODEL_IN_PATH}
)
