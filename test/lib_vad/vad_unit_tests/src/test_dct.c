
#include "vad_unit_tests.h"
#include "vad_dct.h"
#include "vad_mel_scale.h"
#include "dsp.h"

void test_dct(){
    unsigned seed = 4730207;

    int32_t orig_dsp[VAD_N_DCT];
    int32_t orig_vad[VAD_N_DCT];
    int32_t dsp_result[VAD_N_DCT];
    int32_t vad_result[VAD_N_DCT];
    int32_t max = 0x0000000f;

    for(int i = 0; i < 100; i++){

        if((i % 12) == 0) max <<= 3;

        for(int v = 0; v < VAD_N_DCT; v++){
            orig_dsp[v] = pseudo_rand_int(&seed, -max, max);
            orig_vad[v] = orig_dsp[v];
        }

        dsp_dct_forward24(dsp_result, orig_dsp);
        vad_dct_forward24(vad_result, orig_vad);

        int th = 4;
        for(int v = 0; v < VAD_N_DCT; v++){
            int32_t abs_diff = abs(dsp_result[v] - vad_result[v]);
            TEST_ASSERT(abs_diff < th);
        }
    }
}