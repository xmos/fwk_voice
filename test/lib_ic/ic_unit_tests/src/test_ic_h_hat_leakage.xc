// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "ic_unit_tests.h"
#include <stdio.h>
#include <assert.h>
 
void ic_apply_leakage_fp(
    dsp_complex_fp H_hat_fp[IC_Y_CHANNELS][IC_FILTER_PHASES*IC_X_CHANNELS][IC_FD_FRAME_LENGTH],
     double alpha){

    for(int ph=0; ph<IC_X_CHANNELS*IC_FILTER_PHASES; ph++){
        for(int bin=0; bin<IC_FD_FRAME_LENGTH; bin++){
            H_hat_fp[0][ph][bin].re *= alpha;
            H_hat_fp[0][ph][bin].im *= alpha;
        }
    }
}
 

void test_apply_leakage() {
    unsafe{
    ic_state_t state;
    ic_init(&state);

    dsp_complex_fp H_hat_fp[IC_Y_CHANNELS][IC_FILTER_PHASES*IC_X_CHANNELS][IC_FD_FRAME_LENGTH] = {{{{0}}}};
    double alpha_fp = 0;
    
    unsigned seed = 45;
    double max_diff_percentage = 0.0;

    for(int iter=0; iter<(1<<11)/F; iter++) {
 
        for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
            for(int ph=0; ph<IC_FILTER_PHASES*IC_X_CHANNELS;ph++){
                state.H_hat_bfp[ch][ph].exp = sext(att_random_int32(seed), 3) - 31;
                state.H_hat_bfp[ch][ph].hr = att_random_uint32(seed) % 3;                
                for(int i=0; i<IC_FD_FRAME_LENGTH; i++) {
                    state.H_hat_bfp[ch][ph].data[i].re = att_random_int32(seed) >> state.H_hat_bfp[ch][ph].hr;
                    state.H_hat_bfp[ch][ph].data[i].im = att_random_int32(seed) >> state.H_hat_bfp[ch][ph].hr;

                    H_hat_fp[ch][ph][i].re = att_int32_to_double(state.H_hat_bfp[ch][ph].data[i].re, state.H_hat_bfp[ch][ph].exp);
                    H_hat_fp[ch][ph][i].im = att_int32_to_double(state.H_hat_bfp[ch][ph].data[i].im, state.H_hat_bfp[ch][ph].exp);
                }
            }
        }
        //initialise leakage
        for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
            state.ic_adaption_controller_state.leakage_alpha.mant = att_random_uint32(seed) >> 1;//Positive 0 - INT_MAX
            state.ic_adaption_controller_state.leakage_alpha.exp = -31;
            alpha_fp = att_int32_to_double(state.ic_adaption_controller_state.leakage_alpha.mant, state.ic_adaption_controller_state.leakage_alpha.exp);
        }

        for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
            ic_apply_leakage(&state, ych);
            ic_apply_leakage_fp(H_hat_fp, alpha_fp);

            //Since T memory will be overwritten when computing for next y-channel, do error checking now
            for(int ph=0; ph<IC_FILTER_PHASES*IC_X_CHANNELS; ph++) {
                for(int i=0; i<IC_FD_FRAME_LENGTH; i++) {
                    for(int c=0; c<2; c++){
                        double ref_fp = 0;
                        double dut_fp = 0;
                        if(c==0){
                            ref_fp = H_hat_fp[ych][ph][i].re;
                            dut_fp = att_int32_to_double(state.H_hat_bfp[ych][ph].data[i].re, state.H_hat_bfp[ych][ph].exp);

                        }else{
                            ref_fp = H_hat_fp[ych][ph][i].im;
                            dut_fp = att_int32_to_double(state.H_hat_bfp[ych][ph].data[i].im, state.H_hat_bfp[ych][ph].exp);

                        }
                        double diff = (ref_fp - dut_fp);
                        if(diff < 0.0) diff = -diff;
                        double diff_percentage = (diff / (ref_fp < 0.0 ? -ref_fp : ref_fp)) * 100;
                        if(diff_percentage > max_diff_percentage) max_diff_percentage = diff_percentage;
                        if(diff > 0.0002*(ref_fp < 0.0 ? -ref_fp : ref_fp) + pow(10, -8))
                        {
                            printf("%s fail: iter %d, ych %d, ph %d, bin %d\n",c==0 ? "Re" : "Im", iter, ych, ph, i);
                            assert(0);
                        }
                    }
                }
            }
        }
    }
    }
}