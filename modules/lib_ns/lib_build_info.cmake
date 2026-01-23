
set(LIB_NAME lib_ns)
set(LIB_VERSION 0.9.0)
set(LIB_DEPENDENT_MODULES "lib_xcore_math(2.4.0)")
set(LIB_COMPILER_FLAGS -Os -g)
set(LIB_INCLUDES api src)

XMOS_REGISTER_MODULE()
