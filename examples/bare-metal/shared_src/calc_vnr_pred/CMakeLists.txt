
set(APP_NAME avona_shared_src_calc_vnr_pred)

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
        avona::vnr::features
        avona::vnr::inference        
)

add_library(avona::shared::calc_vnr_pred ALIAS ${APP_NAME})
