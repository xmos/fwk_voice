include(fetch_deps.cmake)
add_subdirectory(lib_xcore_math)
add_subdirectory(xscope_fileio)
add_subdirectory(inference)

# tflite_micro - Dependancies

execute_process(
    COMMAND python -c "import xmos_ai_tools.xinterpreters.device as device_lib; import os; print(os.path.dirname(device_lib.__file__), end='')"
    OUTPUT_VARIABLE XMOS_AITOOLSLIB_PATH)

set(ENV{XMOS_AITOOLSLIB_PATH} ${XMOS_AITOOLSLIB_PATH})

if(NOT DEFINED ENV{XMOS_AITOOLSLIB_PATH})
    message(FATAL_ERROR "Path to XMOS AI Tools not set correctly!")
else()
    include($ENV{XMOS_AITOOLSLIB_PATH}/buildfiles/aitoolslib.cmake)
    set(LIB_NAME tflite_micro)
    add_library(${LIB_NAME} STATIC IMPORTED GLOBAL)
    target_compile_definitions(${LIB_NAME} INTERFACE ${XMOS_AITOOLSLIB_DEFINITIONS})
    set_target_properties(${LIB_NAME}  PROPERTIES
        LINKER_LANGUAGE CXX
        IMPORTED_LOCATION ${XMOS_AITOOLSLIB_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${XMOS_AITOOLSLIB_INCLUDES})
endif()