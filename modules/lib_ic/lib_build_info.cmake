
set(LIB_NAME lib_ic)
set(LIB_VERSION 0.9.0)
set(XMOS_DEP_DIR_lib_aec ${CMAKE_CURRENT_LIST_DIR}/../../../fwk_voice/modules)
set(XMOS_DEP_DIR_lib_vnr ${CMAKE_CURRENT_LIST_DIR}/../../../fwk_voice/modules)
set(LIB_DEPENDENT_MODULES "lib_xcore_math(2.4.0)" "lib_aec" "lib_vnr")
set(LIB_COMPILER_FLAGS -Os -g)
set(LIB_INCLUDES api)

XMOS_REGISTER_MODULE()
