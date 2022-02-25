#include "vad_unit_tests.h"
#include "vad_helpers.h"
#include "vad_nn_coefficients.h"
#include "dsp.h"

void test_math_logistics(){
    unsigned seed = 935719;

    int32_t dsp_orig[N_VAD_OUTPUTS];
    int32_t vad_orig[N_VAD_OUTPUTS];
    int32_t dsp_result[N_VAD_OUTPUTS];
    int32_t vad_result[N_VAD_OUTPUTS];
    int32_t max = 0x0000000f;

    for(int i = 0; i < 1000; i++){

        if((i % 120) == 0) max <<= 3;

        int32_t th = 128;
        int32_t abs_diff;

        for(int v = 0; v < N_VAD_OUTPUTS; v++){
            dsp_orig[v] = pseudo_rand_int(&seed, -max, max);
            vad_orig[v] = dsp_orig[v];

            dsp_result[v] = dsp_math_logistics_fast(dsp_orig[v]);
            vad_result[v] = vad_math_logistics_fast(vad_orig[v]);

            abs_diff = abs(dsp_result[v] - vad_result[v]);
            TEST_ASSERT(abs_diff < th);
        }

    }
}