list(APPEND APP_SOURCES 
    "${CMAKE_SOURCE_DIR}/src/ww_model_runner/amazon/model_runner.c"
    "${CMAKE_SOURCE_DIR}/src/ww_model_runner/amazon/mat_mul.c"    
#   "$ENV{WW_PATH}/models/common/WR_250k.en-US.alexa.cpp"
    "$ENV{WW_PATH}/models/common/WR_250k.en-US.alexa-xmos.cpp"
)
list(APPEND APP_INCLUDES "$ENV{WW_PATH}/xs3a")
#target_link_libraries(${TARGET_NAME} "$ENV{WW_PATH}/xs3a/U/libpryon_lite-U.a")
target_link_libraries(${TARGET_NAME} "$ENV{WW_PATH}/xs3a/U/libpryon_lite-U-xmos.a")
add_compile_definitions(
    WW_250K=1
    WW_MODEL_IN_SRAM=0
    USE_SWMEM=1
)
