## Common compile options
add_library(fwk_voice_compile_options INTERFACE)

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
    target_compile_options(fwk_voice_compile_options INTERFACE -fPIC)
endif()

## aec
add_library(fwk_voice_module_aec INTERFACE)
include(aec_schedule_custom_cmake.cmake)
target_sources(fwk_voice_module_aec
    INTERFACE
        src/aec/aec_impl.c
        src/aec/aec_l2_impl.c
        src/aec/aec_priv_impl.c
        src/aec/aec_process_frame.c
        src/aec/aec_schedule_defaults.c
)
target_include_directories(fwk_voice_module_aec
    INTERFACE
        api/aec
        src/aec ## this is needed for the IC
)
target_compile_options(fwk_voice_module_aec
    INTERFACE
        -Os
        -g
)
target_link_libraries(fwk_voice_module_aec
    INTERFACE
        lib_xcore_math
)
## Create an alias
add_library(fwk_voice::aec ALIAS fwk_voice_module_aec)

## vnr
include(${CMAKE_CURRENT_LIST_DIR}/vnr_model.cmake)
## VNR Inference
add_library(fwk_voice_module_lib_vnr STATIC)
file(GLOB_RECURSE VNR_C_SOURCES src/vnr/*.c)
file(GLOB_RECURSE VNR_CXX_SOURCES src/vnr/*.cpp)

target_sources(fwk_voice_module_lib_vnr
    PRIVATE
        ${MODEL_OUT_PATH}.cpp
        ${VNR_CXX_SOURCES}
        ${VNR_C_SOURCES}
)
target_include_directories(fwk_voice_module_lib_vnr PUBLIC api/vnr src/vnr ${MODEL_OUT_DIR})
target_link_libraries(fwk_voice_module_lib_vnr
    PUBLIC
        lib_xcore_math
)
target_link_libraries(fwk_voice_module_lib_vnr
    PRIVATE
	    tflite_micro
)
target_link_libraries(fwk_voice_module_lib_vnr
    PRIVATE
	    fwk_voice_compile_options
)
target_compile_definitions(fwk_voice_module_lib_vnr PRIVATE HEADROOM_CHECK=0)
target_compile_options(fwk_voice_module_lib_vnr
    PRIVATE
        -Os
        -g
)
add_library(fwk_voice::vnr ALIAS fwk_voice_module_lib_vnr)

## ns
add_library(fwk_voice_module_lib_ns STATIC)

target_sources(fwk_voice_module_lib_ns
    PRIVATE
        src/ns/ns_impl.c
        src/ns/ns_low_level.c
)
target_include_directories(fwk_voice_module_lib_ns
    PUBLIC
        api/ns
        src/ns
)
target_compile_options(fwk_voice_module_lib_ns
    PRIVATE
        -Os
        -g
)
target_link_libraries(fwk_voice_module_lib_ns
    PUBLIC
        lib_xcore_math
)
target_link_libraries(fwk_voice_module_lib_ns
    PRIVATE
	    fwk_voice_compile_options
)
## Create an alias
add_library(fwk_voice::ns ALIAS fwk_voice_module_lib_ns)

## agc
add_library(fwk_voice_module_lib_agc STATIC)

target_sources(fwk_voice_module_lib_agc
    PRIVATE
        src/agc/agc_impl.c
)
target_include_directories(fwk_voice_module_lib_agc
    PUBLIC
        api/agc
)
target_compile_options(fwk_voice_module_lib_agc
    PRIVATE
        -Os
        -g
)
target_link_libraries(fwk_voice_module_lib_agc
    PUBLIC
        lib_xcore_math
)
target_link_libraries(fwk_voice_module_lib_agc
    PRIVATE
	    fwk_voice_compile_options
)
## Create an alias
add_library(fwk_voice::agc ALIAS fwk_voice_module_lib_agc)

## ic
file( GLOB_RECURSE    LIB_IC_SOURCES       src/ic/*.c )
## Create library target
add_library(fwk_voice_module_lib_ic STATIC)
target_sources(fwk_voice_module_lib_ic
    PRIVATE
        ${LIB_IC_SOURCES}
)
target_include_directories(fwk_voice_module_lib_ic
    PUBLIC
        api/ic
        src/ic
)
target_compile_options(fwk_voice_module_lib_ic
    PRIVATE
        -Os
        -g
)
target_link_libraries(fwk_voice_module_lib_ic
    PUBLIC
        lib_xcore_math
        fwk_voice::aec
	    fwk_voice::vnr
)
target_link_libraries(fwk_voice_module_lib_ic
    PRIVATE
	    fwk_voice_compile_options
)
## Create an alias
add_library(fwk_voice::ic ALIAS fwk_voice_module_lib_ic)

## adec
file( GLOB_RECURSE    LIB_ADEC_SOURCES       src/adec/*.c )
## Create library target
add_library(fwk_voice_module_lib_adec INTERFACE)
target_sources(fwk_voice_module_lib_adec
    INTERFACE
        ${LIB_ADEC_SOURCES}
)
target_include_directories(fwk_voice_module_lib_adec
    INTERFACE
        api/adec
)
target_compile_options(fwk_voice_module_lib_adec
    INTERFACE
        -Os
        -g
)
target_link_libraries(fwk_voice_module_lib_adec
    INTERFACE
        fwk_voice::aec
)
## Create an alias
add_library(fwk_voice::adec ALIAS fwk_voice_module_lib_adec)

## stage1
add_library(fwk_voice_module_lib_stage1 INTERFACE)

target_sources(fwk_voice_module_lib_stage1
    INTERFACE
        src/stage1/delay_buffer.c
        src/stage1/stage1.c
)

target_include_directories(fwk_voice_module_lib_stage1
    INTERFACE
        api/stage1
        src/stage1
)

target_compile_options(fwk_voice_module_lib_stage1
    INTERFACE
        -Os
        -g
)
target_link_libraries(fwk_voice_module_lib_stage1
    INTERFACE
        fwk_voice::adec
)
## Create an alias
add_library(fwk_voice::stage1 ALIAS fwk_voice_module_lib_stage1)
