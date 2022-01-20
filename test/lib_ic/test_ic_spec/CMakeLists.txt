#same as test_wav_ic so reuse cmake file
set( APP_NAME  test_ic_spec )
set(ADDITIONAL_COMPILE_FLAGS  -DDISABLE_ADAPTION_CONTROLLER=1 ) #Fixed mu
include(../test_wav_ic/CMakeLists.txt)