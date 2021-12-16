// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <assert.h>

#include <unity_fixture.h>



int main(int argc, const char* argv[])
{
    UnityGetCommandLineOptions(argc, argv);
    UnityBegin(argv[0]);

    RUN_TEST_GROUP(ns_update_S);
    RUN_TEST_GROUP(ns_update_p);
    RUN_TEST_GROUP(ns_update_alpha_d_tilde);
    RUN_TEST_GROUP(ns_update_lambda_hat);
    RUN_TEST_GROUP(ns_subtract_lambda_from_frame);
    RUN_TEST_GROUP(ns_minimum);

    return UNITY_END();
}
