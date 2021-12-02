#include <stdint.h>
#include <string.h>
#include <suppression.h>
#include <limits.h>
#include <bfp_init.h>


void bfp_init_zero_int_max(bfp_s32_t * a, int sw){
    if (!sw){
        int32_t data_arr[SUP_PROC_FRAME_BINS] = {0};
        bfp_s32_init(a, data_arr, INT_EXP, SUP_PROC_FRAME_BINS, 0);
        }
    else{
        int32_t data_arr[SUP_PROC_FRAME_BINS] = {INT_MAX};
        bfp_s32_init(a, data_arr, INT_EXP, SUP_PROC_FRAME_BINS, 0);
        }
}

void sup_init_state0(suppression_state_t0 * state){
    memset(state, 0, sizeof(suppression_state_t0));

#if SUP_DEBUG
    for(unsigned y=0;y<SUP_Y_CHANNELS;y++){

        if(!att_is_double_word_aligned((int *)&state->ns[y]))
            printf("Error: struct not aligned state.ns[y] %p\n", &state->ns[y]);
    }

    if(!att_is_double_word_aligned((int *)state->sigma))
        printf("Error: data not aligned state.sigma %p\n", state->sigma);

    if(!att_is_double_word_aligned((int *)state->overlap))
        printf("Error: data not aligned state.overlap %p\n", state.->overlap);
#endif

    for(unsigned y=0;y<SUP_Y_CHANNELS;y++){
        ns_init_state(&state->ns[y]);
        for(unsigned x_ch=0;x_ch<SUP_X_CHANNELS;x_ch++){
            state->sigma[x_ch] = (uint32_t)((double)INT_MAX * 0.00001);
            state->sigma_exp[x_ch]= -32;
        }
    }

    state->bypass = 0;
    state->noise_suppression_enabled = SUP_NS_MCRA_ENABLED;

}

void sup_init_state(suppression_state_t * state){
    memset(state, 0, sizeof(suppression_state_t));

    
    bfp_init_zero_int_max(state->S, 1);
    bfp_init_zero_int_max(state->S_min, 1);
    bfp_init_zero_int_max(state->S_tmp, 1);
    bfp_init_zero_int_max(state->bin_gain, 1);
    bfp_init_zero_int_max(state->p, 0);
    bfp_init_zero_int_max(state->alpha_d_title, 0);
    bfp_init_zero_int_max(state->lambda_hat, 0);
    


    ////////////////////////////////////////////////todo: recalc
    state->reset_period = (unsigned)(16000.0 * 0.15);
    state->alpha_d = (int32_t)((double)INT_MAX * 0.95);
    state->alpha_s = (int32_t)((double)INT_MAX * 0.8);
    state->alpha_p = (int32_t)((double)INT_MAX * 0.2);
    state->noise_floor = (int32_t)((double)INT_MAX * 0.1258925412); // -18dB
    state->delta_q24 = (fixed_s32_t)(1.5);/////////////////////////////////
    state->alpha_gain = INT_MAX;
    

    state->bypass = 0;
    state->noise_suppression_enabled = SUP_NS_MCRA_ENABLED;

}void sup_process_frame(bfp_s32_t * input, suppression_state_t * state, bfp_s32_t * output){
    
}