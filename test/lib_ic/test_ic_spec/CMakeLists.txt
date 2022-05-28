#same as test_wav_ic so reuse cmake file
set( APP_NAME  test_ic_spec )
set(ADDITIONAL_COMPILE_FLAGS  DISABLE_ADAPTION_CONTROLLER=1 ) #Fixed mu and leakage=1.0
include(../test_wav_ic/CMakeLists.txt)
