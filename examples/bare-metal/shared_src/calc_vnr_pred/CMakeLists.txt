
set(APP_NAME fwk_voice_shared_src_calc_vnr_pred)

add_library(${APP_NAME} INTERFACE)
target_sources(${APP_NAME}
    INTERFACE
        src/calc_vnr_pred.c
)
target_include_directories(${APP_NAME}
    INTERFACE
        src
)
target_link_libraries(${APP_NAME}
    INTERFACE
        fwk_voice::vnr::features
        fwk_voice::vnr::inference        
)

add_library(fwk_voice::shared::calc_vnr_pred ALIAS ${APP_NAME})
