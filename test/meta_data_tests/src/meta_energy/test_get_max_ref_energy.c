#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <meta_gen.h>
#include <xs3_math.h>
#include <unity.h>

#include "unity_fixture.h"
#include "../../../shared/pseudo_rand/pseudo_rand.h"
#include "../../../shared/testing/testing.h"

#define EXP -31



TEST_GROUP_RUNNER(get_max_ref_energy){
    RUN_TEST_CASE(get_max_ref_energy, case0);
    RUN_TEST_CASE(get_max_ref_energy, case1);
}

TEST_GROUP(get_max_ref_energy);
TEST_SETUP(get_max_ref_energy) { fflush(stdout); }
TEST_TEAR_DOWN(get_max_ref_energy) {}



double double_get_energy (double * x, unsigned frame_length){
    double result = 0;
    for(int v = 0; v < frame_length; v++){
        result += x[v]*x[v];
    }
    return result;
}


#define frame_length 100
#define NUM_CHAN 2

TEST(get_max_ref_energy, case0)
{
    int32_t ch1[frame_length], ch2[frame_length];
    int32_t * x[NUM_CHAN];
    float_s32_t ch1_fl[frame_length], ch2_fl[frame_length];
    double ch1_db[frame_length], ch2_db[frame_length],actual, expected, t;

    unsigned seed = SEED_FROM_FUNC_NAME();
    x[0] = ch1;
    x[1] = ch2;
    for(int v = 0; v < frame_length; v++){
        ch1[v] = pseudo_rand_int(&seed, 0x1fffffff, 0x7fffffff);
        ch1_fl[v].mant = ch1[v];
        ch1_fl[v].exp = EXP;
        ch1_db[v] = float_s32_to_double(ch1_fl[v]);

        ch2[v] = pseudo_rand_int(&seed, 140000, 180000);
        ch2_fl[v].mant = ch2[v];
        ch2_fl[v].exp = EXP;
        ch2_db[v] = float_s32_to_double(ch2_fl[v]);
    }
    actual = float_s32_to_double(get_max_ref_energy(x, frame_length, NUM_CHAN));

    expected = double_get_energy(ch1_db, frame_length);

    t = double_get_energy(ch2_db,frame_length);
    if (t > expected){expected = t;}
    double diff = expected - actual;
    double error = fabs(diff/expected);
    double thresh = ldexp(1, -19);

    TEST_ASSERT(error < thresh);
}
#undef NUM_CHAN 

#define NUM_CHAN 4

TEST(get_max_ref_energy, case1)
{
    int32_t ch1[frame_length], ch2[frame_length], ch3[frame_length], ch4[frame_length];
    int32_t * x[NUM_CHAN];
    double * y[NUM_CHAN];
    float_s32_t ch1_fl[frame_length], ch2_fl[frame_length], ch3_fl[frame_length], ch4_fl[frame_length];
    double ch1_db[frame_length], ch2_db[frame_length], ch3_db[frame_length], ch4_db[frame_length], actual, expected, t;
    
    unsigned seed = SEED_FROM_FUNC_NAME();
    x[0] = ch1; x[1] = ch2;
    x[2] = ch3; x[3] = ch4;

    y[0] = ch1_db; y[1] = ch2_db;
    y[2] = ch3_db; y[3] = ch4_db;

    for(int v = 0; v < frame_length; v++){
        ch1[v] = pseudo_rand_int(&seed, 0x1aaaaaaa, 0x7aaaaaaa);
        ch1_fl[v].mant = ch1[v];
        ch1_fl[v].exp = EXP;
        ch1_db[v] = float_s32_to_double(ch1_fl[v]);

        ch2[v] = pseudo_rand_int(&seed, 0x1ccccccc, 0x7ccccccc);
        ch2_fl[v].mant = ch2[v];
        ch2_fl[v].exp = EXP;
        ch2_db[v] = float_s32_to_double(ch2_fl[v]);

        ch3[v] = pseudo_rand_int(&seed, 0, 0x1aaaaaaa);
        ch3_fl[v].mant = ch3[v];
        ch3_fl[v].exp = EXP;
        ch3_db[v] = float_s32_to_double(ch3_fl[v]);

        ch4[v] = pseudo_rand_int(&seed, 0x1fffffff, 0x7fffffff);
        ch4_fl[v].mant = ch4[v];
        ch4_fl[v].exp = EXP;
        ch4_db[v] = float_s32_to_double(ch4_fl[v]);
    }
    actual = float_s32_to_double(get_max_ref_energy(x, frame_length, NUM_CHAN));

    expected = double_get_energy(ch1_db, frame_length);

    for(int v = 1; v < NUM_CHAN; v++){
        t = double_get_energy(y[v],frame_length);
        if (t > expected){expected = t;}
    }
    
    double diff = expected - actual;
    double error = fabs(diff/expected);
    double thresh = ldexp(1, -19);

    TEST_ASSERT(error < thresh);
}
#undef NUM_CHAN