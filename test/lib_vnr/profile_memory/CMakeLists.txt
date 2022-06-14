
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(APP_COMPILER_FLAGS
    -Os
    )

set(APP_NAME avona_vnr_test_features_memory)
add_executable(${APP_NAME})
target_sources(${APP_NAME} PRIVATE src/profile_features.c)
target_link_libraries(${APP_NAME} 
    PUBLIC
        avona::vnr::features
        )
target_compile_options(${APP_NAME}
    PRIVATE ${APP_COMPILER_FLAGS} "-target=XCORE-AI-EXPLORER")
target_link_options(${APP_NAME}
    PRIVATE
        -w            
        "-target=${XCORE_TARGET}"
        "-report"
        )

################################################

set(APP_NAME avona_vnr_test_inference_memory)
add_executable(${APP_NAME})
target_sources(${APP_NAME} PRIVATE src/profile_inference.c)
target_link_libraries(${APP_NAME} 
    PUBLIC
        avona::vnr::inference
        )
target_compile_options(${APP_NAME}
    PRIVATE ${APP_COMPILER_FLAGS} "-target=XCORE-AI-EXPLORER")
target_link_options(${APP_NAME}
    PRIVATE
        -w            
        "-target=${XCORE_TARGET}"
        "-report"
        )

################################################

set(APP_NAME avona_vnr_test_full_memory)
add_executable(${APP_NAME})
target_sources(${APP_NAME} PRIVATE src/profile_full.c)
target_link_libraries(${APP_NAME} 
    PUBLIC
        avona::vnr::features
        avona::vnr::inference
        )
target_compile_options(${APP_NAME}
    PRIVATE ${APP_COMPILER_FLAGS} "-target=XCORE-AI-EXPLORER")
target_link_options(${APP_NAME}
    PRIVATE
        -w            
        "-target=${XCORE_TARGET}"
        "-report"
        )



################################################
set(APP_NAME avona_vnr_test_vad_memory)
add_executable(${APP_NAME})
target_sources(${APP_NAME} PRIVATE src/profile_vad.c)
target_link_libraries(${APP_NAME} 
    PUBLIC
        avona::vad
        )
target_compile_options(${APP_NAME}
    PRIVATE ${APP_COMPILER_FLAGS})
target_link_options(${APP_NAME}
    PRIVATE
        -w            
        "-target=${XCORE_TARGET}"
        "-report"
        )

################################################
