
SET(LIB_TFLITE_MICRO_PATH ${CMAKE_BINARY_DIR}/avona_deps/lib_tflite_micro/lib_tflite_micro)
SET(LIB_TFLITE_MICRO_SUBMODULES_PATH ${LIB_TFLITE_MICRO_PATH}/submodules)

include(inference_xcore.cmake)

## **********************************
## Create tflite-micro runtime target
## **********************************
add_library(xcore_sdk_inferencing_tflite_micro INTERFACE)
target_sources(xcore_sdk_inferencing_tflite_micro
    INTERFACE
        ${TFLITE_SOURCES}
)
target_include_directories(xcore_sdk_inferencing_tflite_micro
    INTERFACE
        ${TFLITE_INCLUDES}
)

## Create an alias
add_library(sdk::inferencing::tflite_micro ALIAS xcore_sdk_inferencing_tflite_micro)

#####################################################################################
## ********************
## Create lib_nn target
## ********************
add_library(xcore_sdk_inferencing_lib_nn INTERFACE)


target_include_directories(xcore_sdk_inferencing_lib_nn
    INTERFACE
        ${LIB_NN_INCLUDES}
)

target_sources(xcore_sdk_inferencing_lib_nn
    INTERFACE
        ${LIB_NN_SOURCES}
)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    ## cmake doesn't recognize .S files as assembly by default
    set_source_files_properties(LIB_NN_SOURCES_XCORE_XS3A PROPERTIES LANGUAGE ASM)

    target_sources(xcore_sdk_inferencing_lib_nn
        INTERFACE
            ${LIB_NN_SOURCES_XCORE_XS3A}
    )
endif()

## Create an alias
add_library(sdk::inferencing::lib_nn ALIAS xcore_sdk_inferencing_lib_nn)

#####################################################################################
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    ## ******************************
    ## Create lib_tflite_micro target
    ## ******************************
    add_library(xcore_sdk_inferencing_lib_tflite_micro INTERFACE)

    ## Source files
    set_source_files_properties(TFLIB_SOURCES_ASM PROPERTIES LANGUAGE ASM)

    target_sources(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
            ## XCORE implementations
            ${TFLIB_SOURCES_ASM}
            ${TFLIB_SOURCES}
    )

    target_include_directories(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
            ${TFLIB_INCLUDES}
    )
    target_link_libraries(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
            sdk::inferencing::tflite_micro
            sdk::inferencing::lib_nn
    )

    ## Create an alias
    add_library(sdk::inferencing::lib_tflite_micro ALIAS xcore_sdk_inferencing_lib_tflite_micro)
endif()
