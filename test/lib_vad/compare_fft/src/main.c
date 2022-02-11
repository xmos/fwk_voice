// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <assert.h>

#include <unity_fixture.h>



int main(int argc, const char* argv[])
{
    UnityGetCommandLineOptions(argc, argv);
    UnityBegin(argv[0]);

    RUN_TEST_GROUP(test_compare_fft);
    RUN_TEST_GROUP(test_compare_fft_mfcc);


    return UNITY_END();
}
