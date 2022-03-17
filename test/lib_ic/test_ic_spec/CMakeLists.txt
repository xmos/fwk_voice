#same as test_wav_ic so reuse cmake file
set( APP_NAME  test_ic_spec )
set(ADDITIONAL_COMPILE_FLAGS  IC_SPEC_TEST=1 ) #Fixed mu and leakage=1.0
include(../test_wav_ic/CMakeLists.txt)
