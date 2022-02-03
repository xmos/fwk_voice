// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <assert.h>

#include <unity_fixture.h>



int main(int argc, const char* argv[])
{
    UnityGetCommandLineOptions(argc, argv);
    UnityBegin(argv[0]);

    RUN_TEST_GROUP(sup_pack_input);
    RUN_TEST_GROUP(sup_apply_window);
    RUN_TEST_GROUP(sup_form_output);
    RUN_TEST_GROUP(sup_rescale_vector);


    return UNITY_END();
}
