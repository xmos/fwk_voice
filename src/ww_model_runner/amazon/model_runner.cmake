list(APPEND APP_SOURCES "${CMAKE_SOURCE_DIR}/src/ww_model_runner/amazon/model_runner.c")
list(APPEND APP_INCLUDES "$ENV{WW_PATH}/xs3a")
target_link_libraries(${TARGET_NAME} "$ENV{WW_PATH}/xs3a/U/libpryon_lite-U.a")
add_compile_definitions(
    WW_250K=0
)
