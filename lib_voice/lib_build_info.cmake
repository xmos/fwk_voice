set(LIB_NAME lib_voice)
set(LIB_VERSION 0.8.1)
set(LIB_DEPENDENT_MODULES "lib_xcore_math(2.4.0)")

set(LIB_COMPILER_FLAGS -g -Os -DHEADROOM_CHECK=0)
set(LIB_CXX_SRCS "")
include(${CMAKE_CURRENT_LIST_DIR}/vnr_model.cmake)
file(RELATIVE_PATH MODEL_OUT_DIR_REL ${CMAKE_CURRENT_LIST_DIR} ${MODEL_OUT_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/aec_schedule.cmake)

set(LIB_INCLUDES
    api
    api/adec
    api/aec
    src/aec
    api/agc
    api/ic
    src/ic
    api/ns
    src/ns
    api/stage1
    api/vnr
    src/vnr
    ${MODEL_OUT_DIR_REL}
)

file(GLOB VNR_CXX_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/vnr/*.cpp")
file(RELATIVE_PATH VNR_MODEL_SOURCES ${CMAKE_CURRENT_LIST_DIR} ${MODEL_OUT_PATH}.cpp)

list(APPEND LIB_CXX_SRCS ${VNR_MODEL_SOURCES} ${VNR_CXX_SOURCES})

XMOS_REGISTER_MODULE()

# Link aitools with the targets
foreach(target ${APP_BUILD_TARGETS})
if(AUTOGEN_DIR)
    foreach(target ${APP_BUILD_TARGETS})
        target_sources(${target} PRIVATE ${AUTOGEN_SOURCES})
        target_include_directories(${target} PRIVATE ${AUTOGEN_DIR})
        target_compile_definitions(${target} PRIVATE __aec_conf_h_exists__)
    endforeach()
endif()
    target_link_libraries(${target} PRIVATE tflite_micro)
endforeach()
