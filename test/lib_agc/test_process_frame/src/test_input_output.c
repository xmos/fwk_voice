// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// This test checks that agc_process_frame() can be safely performed in-place on the input
// array, and additionally that the input array is not altered when a separate output array
// is provided. Two identically configured AGC instances are used for this test. For each
// iteration, a random frame of data is generated, and stored in an array that will not be
// passed to either instance. Then one AGC operates in-place on a copy of the input, and
// the other writes its output into a new buffer. The output from the in-place operation
// must match the other output, and the input to the non-in-place must be unchanged when
// compared with the original input buffer that was stored.

void test_input_output() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t input0[AGC_FRAME_ADVANCE];
    int32_t input1[AGC_FRAME_ADVANCE];
    int32_t output1[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp;

    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    // Meta-data can be shared between AGC instances
    agc_meta_data_t md;

    agc_state_t agc0;
    agc_init(&agc0, &AGC_PROFILE_COMMS);

    agc_state_t agc1;
    agc_init(&agc1, &AGC_PROFILE_COMMS);

    // Random seed
    unsigned seed = 34090;

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input[idx] = pseudo_rand_int32(&seed);
            input0[idx] = input[idx];
            input1[idx] = input[idx];
        }

        // Random scale from zero to one
        float_s32_t scale = {pseudo_rand_uint32(&seed), -32};
        bfp_s32_headroom(&input_bfp);
        float_s32_t in_power = float_s64_to_float_s32(bfp_s32_energy(&input_bfp));

        // Set meta-data to random values
        md.vnr_flag = pseudo_rand_uint8(&seed) & 1;    // Boolean
        md.aec_ref_power = float_s32_mul(in_power, scale);
        md.aec_corr_factor = (float_s32_t){pseudo_rand_uint32(&seed), -32};

        agc_process_frame(&agc0, input0, input0, &md);

        agc_process_frame(&agc1, output1, input1, &md);

        TEST_ASSERT_EQUAL_INT32_ARRAY(input, input1, AGC_FRAME_ADVANCE);
        TEST_ASSERT_EQUAL_INT32_ARRAY(output1, input0, AGC_FRAME_ADVANCE);
    }
}
