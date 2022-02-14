// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <assert.h>

#include <unity_fixture.h>



int main(int argc, const char* argv[])
{
    UnityGetCommandLineOptions(argc, argv);
    UnityBegin(argv[0]);

    RUN_TEST_GROUP(ns_pack_input);
    RUN_TEST_GROUP(ns_apply_window);
    RUN_TEST_GROUP(ns_form_output);
    RUN_TEST_GROUP(ns_rescale_vector);


    return UNITY_END();
}
