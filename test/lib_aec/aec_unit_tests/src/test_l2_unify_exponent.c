// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

//TODO MODIFY TO TEST FOR POSITIVE AND NEGATIVE EXPONENTS!!!

#define NUM_CHUNKS (20)
#define NUM_SUBGROUPS (3)
#define LENGTH_PER_SUBGROUP (500)
void test_bfp_complex_s32_l2_unify_exponent() {
    complex_s32_t mem[NUM_SUBGROUPS][LENGTH_PER_SUBGROUP];
    bfp_complex_s32_t chunks[NUM_CHUNKS];
    int chunk_subgroup_mapping[NUM_CHUNKS];
    
    complex_double_t mem_float[NUM_SUBGROUPS][LENGTH_PER_SUBGROUP];
    
    int32_t max_diff = 0;
    unsigned seed = 34;
    int remaining_length[NUM_SUBGROUPS];
    int min_reqd_headroom[NUM_SUBGROUPS];
    int null_mapping; //null_mapping = 1 => unify everything without looking at subgroups
    //null_mapping = 0 => unify according to subgroups
    for(int iter=0; iter<(1<<12)/F; iter++) {
        null_mapping = pseudo_rand_uint32(&seed) % 2;
        for(int i=0; i<NUM_SUBGROUPS; i++) {
            remaining_length[i] = LENGTH_PER_SUBGROUP;
            min_reqd_headroom[i] = pseudo_rand_uint32(&seed) % 4;
        }
        //Setup input
        for(int c=0; c<NUM_CHUNKS; c++) {
            int subgroup = pseudo_rand_uint32(&seed) % NUM_SUBGROUPS; //which subgroup the chunk belongs to
            chunk_subgroup_mapping[c] = subgroup;
            chunks[c].exp = pseudo_rand_int(&seed, -31, 32);
            chunks[c].hr = (pseudo_rand_uint32(&seed) % 4);
            //generate lengths such that total adds to LENGTH_PER_SUBGROUP            
            if(remaining_length[subgroup]) {
                chunks[c].length = pseudo_rand_uint32(&seed) % remaining_length[subgroup];
                chunks[c].data = &mem[subgroup][LENGTH_PER_SUBGROUP - remaining_length[subgroup]];
                //generate data
                for(int ii=0; ii<chunks[c].length; ii++) {
                    chunks[c].data[ii].re = pseudo_rand_int32(&seed) >> chunks[c].hr;
                    chunks[c].data[ii].im = pseudo_rand_int32(&seed) >> chunks[c].hr;
                    //keep a copy in float array
                    mem_float[subgroup][LENGTH_PER_SUBGROUP - remaining_length[subgroup] + ii].re = ldexp(chunks[c].data[ii].re, chunks[c].exp);
                    mem_float[subgroup][LENGTH_PER_SUBGROUP - remaining_length[subgroup] + ii].im = ldexp(chunks[c].data[ii].im, chunks[c].exp);
                }
                remaining_length[subgroup] -= chunks[c].length;
            }
            else {
                chunks[c].length = 0;
            }
            //printf("chunk %d, subgroup %d. exp %d, hr %d, length %d\n", c, subgroup, chunks[c].exp, chunks[c].hr, chunks[c].length);
        }
        
        int final_exp, final_hr;
        bfp_complex_s32_t unified[NUM_SUBGROUPS];
        if(!null_mapping) {
            for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
                aec_l2_bfp_complex_s32_unify_exponent(chunks, &final_exp, &final_hr, chunk_subgroup_mapping, NUM_CHUNKS, sb, min_reqd_headroom[sb]);
                //printf("%d\n",final_exp);
                if(final_exp == INT_MIN) {
                    assert(0);
                }
                bfp_complex_s32_init(&unified[sb], &mem[sb][0], final_exp, LENGTH_PER_SUBGROUP-remaining_length[sb], 0);
                unified[sb].hr = final_hr;
                //printf("subgroup %d, min_reqd_headroom %d: final_exp %d, final_hr %d\n",sb, min_reqd_headroom[sb], final_exp, final_hr);
            }
        }
        else
        {
            aec_l2_bfp_complex_s32_unify_exponent(chunks, &final_exp, &final_hr, NULL, NUM_CHUNKS, 0, min_reqd_headroom[0]);
            //printf("%d\n",final_exp);
            if(final_exp == INT_MIN) {
                assert(0);
            }
            for(int sb=1; sb<NUM_SUBGROUPS; sb++) {
                min_reqd_headroom[sb] = min_reqd_headroom[0];
            }
            for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
                bfp_complex_s32_init(&unified[sb], &mem[sb][0], final_exp, LENGTH_PER_SUBGROUP-remaining_length[sb], 0);
                unified[sb].hr = final_hr;
                //printf("subgroup %d, min_reqd_headroom %d: final_exp %d, final_hr %d\n",sb, min_reqd_headroom[sb], final_exp, final_hr);
            }
        }

        //check output
        for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
            for(int i=0; i<unified[sb].length; i++) {
                int32_t ref_int = double_to_int32( mem_float[sb][i].re, unified[sb].exp);
                int32_t dut_int = unified[sb].data[i].re;
                int32_t diff = ref_int - dut_int;
                if(diff < 0) diff = -diff;
                if(diff > max_diff) max_diff = diff;
                TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<1, ref_int, dut_int, "unify broke for bfp_complex_s32 re");

                ref_int = double_to_int32( mem_float[sb][i].im, unified[sb].exp);
                dut_int = unified[sb].data[i].im;
                diff = ref_int - dut_int;
                if(diff < 0) diff = -diff;
                if(diff > max_diff) max_diff = diff;
                TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<1, ref_int, dut_int, "unify broke for bfp_complex_s32 im");
            }
        }
        //check headroom
        for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
            int actual_headroom = bfp_complex_s32_headroom(&unified[sb]);
            //printf("hr: dut %d, actual %d\n", unified[sb].hr, actual_headroom);
            if(unified[sb].hr > actual_headroom) {
                printf("iter %d, bfp_complex_s32, actual headroom is less than the reported output headroom\n", iter);
                assert(0);
            }
            if(unified[sb].hr < min_reqd_headroom[sb]) {
                printf("iter %d, bfp_complex_s32, output headroom is less than the minimum required headroom\n", iter);
                assert(0);
            }
        }
    }
    printf("max_diff = %ld\n",max_diff);
}

void test_bfp_s32_l2_unify_exponent() {
    int32_t mem[NUM_SUBGROUPS][LENGTH_PER_SUBGROUP];
    bfp_s32_t chunks[NUM_CHUNKS];
    int chunk_subgroup_mapping[NUM_CHUNKS];
    
    double mem_float[NUM_SUBGROUPS][LENGTH_PER_SUBGROUP];
    
    int32_t max_diff = 0;
    unsigned seed = 34;
    int remaining_length[NUM_SUBGROUPS];
    int min_reqd_headroom[NUM_SUBGROUPS];
    int null_mapping; //null_mapping = 1 => unify everything without looking at subgroups
    //null_mapping = 0 => unify according to subgroups
    for(int iter=0; iter<1<<12; iter++) {
        null_mapping = pseudo_rand_uint32(&seed) % 2;
        for(int i=0; i<NUM_SUBGROUPS; i++) {
            remaining_length[i] = LENGTH_PER_SUBGROUP;
            min_reqd_headroom[i] = pseudo_rand_uint32(&seed) % 4;
        }
        //Setup input
        for(int c=0; c<NUM_CHUNKS; c++) {
            int subgroup = pseudo_rand_uint32(&seed) % NUM_SUBGROUPS; //which subgroup the chunk belongs to
            chunk_subgroup_mapping[c] = subgroup;
            chunks[c].exp = pseudo_rand_int(&seed, -31, 32);
            chunks[c].hr = (pseudo_rand_uint32(&seed) % 4);
            //how to generate lengths such that total adds to LENGTH_PER_SUBGROUP
            if(remaining_length[subgroup]) {
                chunks[c].length = pseudo_rand_uint32(&seed) % remaining_length[subgroup];
                chunks[c].data = &mem[subgroup][LENGTH_PER_SUBGROUP - remaining_length[subgroup]];
                //generate data
                for(int ii=0; ii<chunks[c].length; ii++) {
                    chunks[c].data[ii] = pseudo_rand_int32(&seed) >> chunks[c].hr;
                    //keep a copy in float array
                    mem_float[subgroup][LENGTH_PER_SUBGROUP - remaining_length[subgroup] + ii] = ldexp(chunks[c].data[ii], chunks[c].exp);
                }
                remaining_length[subgroup] -= chunks[c].length;
            }
            else {
                chunks[c].length = 0;
            }
            //printf("chunk %d, subgroup %d. exp %d, hr %d, length %d\n", c, subgroup, chunks[c].exp, chunks[c].hr, chunks[c].length);
        }
        
        int final_exp, final_hr;
        bfp_s32_t unified[NUM_SUBGROUPS];
        if(!null_mapping) {
            for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
                aec_l2_bfp_s32_unify_exponent(chunks, &final_exp, &final_hr, chunk_subgroup_mapping, NUM_CHUNKS, sb, min_reqd_headroom[sb]);
                if(final_exp == INT_MIN) {
                    assert(0);
                }
                bfp_s32_init(&unified[sb], &mem[sb][0], final_exp, LENGTH_PER_SUBGROUP-remaining_length[sb], 0);
                unified[sb].hr = final_hr;
                //printf("subgroup %d, min_reqd_headroom %d: final_exp %d, final_hr %d\n",sb, min_reqd_headroom[sb], final_exp, final_hr);
            }
        }
        else
        {
            aec_l2_bfp_s32_unify_exponent(chunks, &final_exp, &final_hr, NULL, NUM_CHUNKS, 0, min_reqd_headroom[0]);
            if(final_exp == INT_MIN) {
                assert(0);
            }
            for(int sb=1; sb<NUM_SUBGROUPS; sb++) {
                min_reqd_headroom[sb] = min_reqd_headroom[0];
            }
            for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
                bfp_s32_init(&unified[sb], &mem[sb][0], final_exp, LENGTH_PER_SUBGROUP-remaining_length[sb], 0);
                unified[sb].hr = final_hr;
                //printf("subgroup %d, min_reqd_headroom %d: final_exp %d, final_hr %d\n",sb, min_reqd_headroom[sb], final_exp, final_hr);
            }
        }

        //check output
        for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
            for(int i=0; i<unified[sb].length; i++) {
                int32_t ref_int = double_to_int32( mem_float[sb][i], unified[sb].exp);
                int32_t dut_int = unified[sb].data[i];
                int32_t diff = ref_int - dut_int;
                if(diff < 0) diff = -diff;
                if(diff > max_diff) max_diff = diff;
                TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<1, ref_int, dut_int, "unify broke for bfp_s32");
            }
        }
        //check headroom
        for(int sb=0; sb<NUM_SUBGROUPS; sb++) {
            int actual_headroom = bfp_s32_headroom(&unified[sb]);
            //printf("hr: dut %d, actual %d\n", unified[sb].hr, actual_headroom);
            if(unified[sb].hr > actual_headroom) {
                printf("iter %d, bfp_s32, actual headroom is less than the reported output headroom\n", iter);
                assert(0);
            }
        }
    }
    printf("max_diff = %ld\n",max_diff);
}
