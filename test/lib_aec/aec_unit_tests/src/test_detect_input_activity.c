// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "aec_api.h"
int detect_input_activity_fp(double (*input)[AEC_FRAME_ADVANCE], double active_threshold, int channels) {
    for(int ch=0; ch<channels; ch++) {
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            if(fabs(input[ch][i]) > active_threshold) {
                return 1;    
            }
        }
    }
    return 0;
}

float_s32_t float_s32_use_exponent(float_s32_t a, int exp) {
    float_s32_t out;
    out.mant = a.mant;
    out.exp = a.exp;

    bfp_s32_t out_bfp;
    bfp_s32_init(&out_bfp, &out.mant, out.exp, 1, 0);
    
    bfp_s32_use_exponent(&out_bfp, exp);
    return out;
}

#define CHANNELS (4)
void test_detect_input_activity() {
    int32_t DWORD_ALIGNED dut[CHANNELS][AEC_FRAME_ADVANCE];
    uint32_t dut_active;
    double ref[CHANNELS][AEC_FRAME_ADVANCE];
    uint32_t ref_active;

    double active_threshold_dB = -60;
    double ref_active_threshold = pow(10, active_threshold_dB/20.0);
    float_s32_t dut_active_threshold = f64_to_float_s32(ref_active_threshold);

    uint32_t seed = 43554;

    // Test: all values 0.activity should be 0
    for(int ch=0; ch<CHANNELS; ch++) {
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            ref[ch][i] = 0.0;
            dut[ch][i] = 0;
        }
    }
    ref_active = detect_input_activity_fp(ref, ref_active_threshold, CHANNELS);
    dut_active = aec_detect_input_activity(dut, dut_active_threshold, CHANNELS);
    if(dut_active != 0) {
        printf("test: all values 0. FAIL. dut detected active\n");
        assert(0);
    }

    // Test: all values between [-thresh,thresh]. active should be 0.
    bfp_s32_t dut_bfp[CHANNELS];
    for(int ch=0; ch<CHANNELS; ch++) {
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            dut[ch][i] = pseudo_rand_int32(&seed);
        }
        bfp_s32_init(&dut_bfp[ch], &dut[ch][0], -31, AEC_FRAME_ADVANCE, 0);
        bfp_s32_clip(&dut_bfp[ch], &dut_bfp[ch], -dut_active_threshold.mant, dut_active_threshold.mant, dut_active_threshold.exp);
        bfp_s32_use_exponent(&dut_bfp[ch], -31);
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            ref[ch][i] = ldexp(dut[ch][i], -31);
        }
    }
    ref_active = detect_input_activity_fp(ref, ref_active_threshold, CHANNELS);
    dut_active = aec_detect_input_activity(dut, dut_active_threshold, CHANNELS);
    if(dut_active != 0) {
        printf("test: all values between -thresh,+thresh. FAIL. dut detected active\n");
        assert(0);
    }

    // Test: 1 value > thresh. active should be 1
    for(int ch=0; ch<CHANNELS; ch++) {
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            dut[ch][i] = pseudo_rand_int32(&seed);
        }
        bfp_s32_init(&dut_bfp[ch], &dut[ch][0], -31, AEC_FRAME_ADVANCE, 0);
        bfp_s32_clip(&dut_bfp[ch], &dut_bfp[ch], -dut_active_threshold.mant, dut_active_threshold.mant, dut_active_threshold.exp);
        bfp_s32_use_exponent(&dut_bfp[ch], -31);
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            ref[ch][i] = ldexp(dut[ch][i], -31);
        }
    }
    //make one value above threshold.
    int m = (pseudo_rand_int32(&seed) & 0x7fffffff) % CHANNELS;
    int n = (pseudo_rand_int32(&seed) & 0x7fffffff) % AEC_FRAME_ADVANCE;
    dut[m][n] = (float_s32_use_exponent(dut_active_threshold, -31)).mant + 0x10; //Set to something slightly higher than threshold
    ref[m][n] = ldexp(dut[m][n], -31);
    ref_active = detect_input_activity_fp(ref, ref_active_threshold, CHANNELS);
    dut_active = aec_detect_input_activity(dut, dut_active_threshold, CHANNELS);
    if(dut_active != 1) {
        printf("test: 1 value above thresh. FAIL. dut detected inactive\n");
        assert(0);
    }

    //Test: one value < -thresh. active should be 1
    for(int ch=0; ch<CHANNELS; ch++) {
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            dut[ch][i] = pseudo_rand_int32(&seed);
        }
        bfp_s32_init(&dut_bfp[ch], &dut[ch][0], -31, AEC_FRAME_ADVANCE, 0);
        bfp_s32_clip(&dut_bfp[ch], &dut_bfp[ch], -dut_active_threshold.mant, dut_active_threshold.mant, dut_active_threshold.exp);
        bfp_s32_use_exponent(&dut_bfp[ch], -31);
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            ref[ch][i] = ldexp(dut[ch][i], -31);
        }
    }
    //make one value above threshold
    m = (pseudo_rand_int32(&seed) & 0x7fffffff) % CHANNELS;
    n = (pseudo_rand_int32(&seed) & 0x7fffffff) % AEC_FRAME_ADVANCE;
    dut[m][n] = -(float_s32_use_exponent(dut_active_threshold, -31)).mant - 0x10; //Set to something slightly smaller than -threshold
    ref[m][n] = ldexp(dut[m][n], -31);
    ref_active = detect_input_activity_fp(ref, ref_active_threshold, CHANNELS);
    dut_active = aec_detect_input_activity(dut, dut_active_threshold, CHANNELS);
    if(dut_active != 1) {
        printf("test: 1 value above thresh. FAIL. dut detected inactive\n");
        assert(0);
    }
}
