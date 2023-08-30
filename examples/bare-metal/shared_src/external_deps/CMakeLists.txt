include(fetch_deps.cmake)
add_subdirectory(lib_xcore_math)
add_subdirectory(xscope_fileio)
add_subdirectory(inference)

# tflite_micro - Dependancies

# Get path of xmos ai tools
set(CMD "\
import os; \
import xmos_ai_tools.xinterpreters.device as dl; \
print(os.path.dirname(dl.__file__)) \
")

execute_process(
    COMMAND python -c "${CMD}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE XMOS_AITOOLSLIB_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# tflite_micro - add dependancy
set(XMOS_AITOOLSLIB_PATH_CMAKE "${XMOS_AITOOLSLIB_PATH}/buildfiles/aitoolslib.cmake")

if(XMOS_AITOOLSLIB_PATH STREQUAL "")
    message(FATAL_ERROR "Path to XMOS AI tools NOT found")
elseif(NOT EXISTS  ${XMOS_AITOOLSLIB_PATH_CMAKE})
    message(FATAL_ERROR "Cmake NOT found in this path")
else()
    message("Found python package xmos-ai-tools at: ${XMOS_AITOOLSLIB_PATH}")
    include(${XMOS_AITOOLSLIB_PATH_CMAKE})
    set(LIB_NAME tflite_micro)
    add_library(${LIB_NAME} STATIC IMPORTED GLOBAL)
    target_compile_definitions(${LIB_NAME} INTERFACE ${XMOS_AITOOLSLIB_DEFINITIONS})
    set_target_properties(${LIB_NAME}  PROPERTIES
        LINKER_LANGUAGE CXX
        IMPORTED_LOCATION ${XMOS_AITOOLSLIB_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${XMOS_AITOOLSLIB_INCLUDES})
endif()