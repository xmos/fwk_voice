set(LIB_NAME lib_stage1)
set(LIB_VERSION 0.9.0)
set(XMOS_DEP_DIR_lib_adec ${CMAKE_CURRENT_LIST_DIR}/../../../fwk_voice/modules)
set(LIB_DEPENDENT_MODULES "lib_adec")
set(LIB_COMPILER_FLAGS -Os -g)
set(LIB_INCLUDES api)

XMOS_REGISTER_MODULE()
