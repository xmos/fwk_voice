// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <fdaf_state.h>
#define FDAF_WINDOW_EXP (-31)
#define FDAF_INPUT_EXP (-31)

// Hanning window structure used in the windowing operation done to remove discontinuities from the filter error
static const fixed_s32_t WOLA_window_q31[FDAF_UNUSED_TAPS_PER_PHASE * 2] = {
       4861986,   19403913,   43494088,   76914346,  119362028,  170452721,  229723740,  296638317,
     370590464,  450910459,  536870911,  627693349,  722555272,  820597594,  920932429, 1022651130,
    1124832516, 1226551217, 1326886052, 1424928374, 1519790297, 1610612735, 1696573187, 1776893182,
    1850845329, 1917759906, 1977030925, 2028121618, 2070569300, 2103989558, 2128079733, 2142621660
};

static const fixed_s32_t WOLA_window_flpd_q31[FDAF_UNUSED_TAPS_PER_PHASE * 2] = {
    2142621660, 2128079733, 2103989558, 2070569300, 2028121618, 1977030925, 1917759906, 1850845329, 
    1776893182, 1696573187, 1610612735, 1519790297, 1424928374, 1326886052, 1226551217, 1124832516, 
    1022651130, 920932429, 820597594, 722555272, 627693349, 536870911, 450910459, 370590464, 
    296638317, 229723740, 170452721, 119362028, 76914346, 43494088, 19403913, 4861986, 
};

void fdaf_create_output(
        bfp_s32_t * output,
        bfp_s32_t * overlap,
        bfp_s32_t * error)
{
    bfp_s32_t win, win_flpd;
    bfp_s32_init(&win, (int32_t*)&WOLA_window_q31[0], FDAF_WINDOW_EXP, (FDAF_UNUSED_TAPS_PER_PHASE * 2), 0);
    bfp_s32_init(&win_flpd, (int32_t*)&WOLA_window_flpd_q31[0], FDAF_WINDOW_EXP, (FDAF_UNUSED_TAPS_PER_PHASE * 2) , 0);

    //zero first 240 samples
    memset(error->data, 0, FDAF_FRAME_ADVANCE*sizeof(int32_t));

    bfp_s32_t chunks[2];
    bfp_s32_init(&chunks[0], &error->data[FDAF_FRAME_ADVANCE], error->exp, FDAF_UNUSED_TAPS_PER_PHASE * 2, 1); //240-272 fwd win
    bfp_s32_init(&chunks[1], &error->data[2 * FDAF_FRAME_ADVANCE], error->exp, FDAF_UNUSED_TAPS_PER_PHASE * 2, 1); //480-512 flpd win

    //window error
    bfp_s32_mul(&chunks[0], &chunks[0], &win);
    bfp_s32_mul(&chunks[1], &chunks[1], &win_flpd);
    //Bring the windowed portions back to the format to the non-windowed region.
    //Here, we're assuming that the window samples are less than 1 so that windowed region can be safely brought to format of non-windowed portion without risking saturation
    bfp_s32_use_exponent(&chunks[0], error->exp);
    bfp_s32_use_exponent(&chunks[1], error->exp);
    uint32_t min_hr = (chunks[0].hr < chunks[1].hr) ? chunks[0].hr : chunks[1].hr;
    min_hr = (min_hr < error->hr) ? min_hr : error->hr;
    error->hr = min_hr;
    
    //copy error to output
    if(output->data != NULL) {
        memcpy(output->data, &error->data[FDAF_FRAME_ADVANCE], FDAF_FRAME_ADVANCE*sizeof(int32_t));
        output->length = FDAF_FRAME_ADVANCE;
        output->exp = error->exp;
        output->hr = error->hr;

        //overlap add
        //split output into 2 chunks. chunk[0] with first 32 samples of output. chunk[1] has rest of the 240-32 samples of output
        bfp_s32_init(&chunks[0], &output->data[0], output->exp, FDAF_UNUSED_TAPS_PER_PHASE * 2, 1);
        bfp_s32_init(&chunks[1], &output->data[FDAF_UNUSED_TAPS_PER_PHASE * 2], output->exp, FDAF_FRAME_ADVANCE - (FDAF_UNUSED_TAPS_PER_PHASE * 2), 1);

        //Add previous frame's overlap to first 32 samples of output
        bfp_s32_add(&chunks[0], &chunks[0], overlap);
        bfp_s32_use_exponent(&chunks[0], FDAF_INPUT_EXP); //bring the overlapped-added part back to 1.31 since output is same format as input
        bfp_s32_use_exponent(&chunks[1], FDAF_INPUT_EXP); //bring the rest of output to 1.31 since output is same format as input
        output->hr = (chunks[0].hr < chunks[1].hr) ? chunks[0].hr : chunks[1].hr;
    }
    
    //update overlap
    memcpy(overlap->data, &error->data[2 * FDAF_FRAME_ADVANCE], (FDAF_UNUSED_TAPS_PER_PHASE * 2) * sizeof(int32_t));
    overlap->hr = error->hr;
    overlap->exp = error->exp;
}

