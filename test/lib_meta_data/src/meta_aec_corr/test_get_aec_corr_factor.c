#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <xs3_math.h>
#include <bfp_math.h>

#include <meta_gen.h>
#include <unity.h>

#include "unity_fixture.h"
#include "../../../shared/pseudo_rand/pseudo_rand.h"
#include "../../../shared/testing/testing.h"

#define EXP  -31
#define AP_FRAME_ADVANCE (240)
#define AEC_PROC_FRAME_LENGTH (512)


TEST_GROUP_RUNNER(get_aec_corr_factor){
    RUN_TEST_CASE(get_aec_corr_factor, case0);
}

TEST_GROUP(get_aec_corr_factor);
TEST_SETUP(get_aec_corr_factor) { fflush(stdout); }
TEST_TEAR_DOWN(get_aec_corr_factor) {}

double dot_prod(double * a, double * b, unsigned length){
    double acc = 0;
    for (int v = 0; v < length; v++){
        acc += a[v] * b[v];
    }
    return acc;
}

TEST(get_aec_corr_factor, case0){
    const int FRAME_WINDOW = AEC_PROC_FRAME_LENGTH % AP_FRAME_ADVANCE;
    int32_t mic_int[AP_FRAME_ADVANCE],  y_hat_int[AP_FRAME_ADVANCE];
    float_s32_t  mic_fl[AP_FRAME_ADVANCE], y_hat_fl[AP_FRAME_ADVANCE];
    double mic_db[AP_FRAME_ADVANCE], y_hat_db[AP_FRAME_ADVANCE], actual, expected;
    double mic_db_abs[AP_FRAME_ADVANCE], y_hat_db_abs[AP_FRAME_ADVANCE],numenator, denominator;
    bfp_s32_t y_hat;
    unsigned seed = SEED_FROM_FUNC_NAME();

    for(int v = 0; v <AP_FRAME_ADVANCE; v++)
    {
        mic_int[v] = pseudo_rand_int(&seed, 0xafffffff, 0x7fffffff);
        mic_fl[v].mant = mic_int[v];
        mic_fl[v].exp = EXP;
        mic_db[v] = float_s32_to_double(mic_fl[v]);

        y_hat_int[v] = pseudo_rand_int(&seed, 0xafffffff, 0x7fffffff);
        y_hat_fl[v].mant = y_hat_int[v];
        y_hat_fl[v].exp = EXP;
        y_hat_db[v] = float_s32_to_double(y_hat_fl[v]);
    }

    bfp_s32_init(&y_hat, y_hat_int, EXP, AP_FRAME_ADVANCE, 1);

    actual = float_s32_to_double(get_aec_corr_factor(mic_int, &y_hat));

    numenator = dot_prod(mic_db, y_hat_db, AP_FRAME_ADVANCE - FRAME_WINDOW);

    for(int v = 0; v < AP_FRAME_ADVANCE; v++){
        mic_db_abs[v] = fabs(mic_db[v]);
        y_hat_db_abs[v] = fabs(y_hat_db[v]);
    }

    denominator = dot_prod(mic_db_abs, y_hat_db_abs, AP_FRAME_ADVANCE - FRAME_WINDOW);

    expected = fabs(numenator)/denominator;

    double diff = expected - actual;
    double error = fabs(diff/expected);
    double thresh = ldexp(1, -19);

    TEST_ASSERT(error < thresh);
}

