// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_priv.h"
#include "xmath/xmath.h"

void aec_priv_main_init(
        aec_state_t *state,
        aec_shared_state_t *shared_state,
        uint8_t *mem_pool,
        unsigned num_y_channels,
        unsigned num_x_channels,
        unsigned num_phases)
{ 
    memset(state, 0, sizeof(aec_state_t));
    //reset shared_state. Only done in main_init()
    memset(shared_state, 0, sizeof(aec_shared_state_t));

    uint8_t *available_mem_start = (uint8_t*)mem_pool;

    state->shared_state = shared_state;
    //Initialise number of y and x channels
    state->shared_state->num_y_channels = num_y_channels;
    state->shared_state->num_x_channels = num_x_channels;
    //Initialise number of phases
    state->num_phases = num_phases;

    //y
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->shared_state->y[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH), 0); //input data is 1.31 so initialising with exp AEC_INPUT_EXP
        available_mem_start += ((AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING)*sizeof(int32_t)); //2 extra samples of memory allocated. state->shared_state->y[ch].length is still AEC_PROC_FRAME_LENGTH though
    }
    //x
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->shared_state->x[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH), 0); //input data is 1.31 so initialising with exp -31
        available_mem_start += ((AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING)*sizeof(int32_t)); //2 extra samples of memory allocated. state->shared_state->x[ch].length is still AEC_PROC_FRAME_LENGTH though
    }
    //prev_y
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->shared_state->prev_y[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE), 0); //input data is 1.31 so initialising with exp -31
        available_mem_start += ((AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE)*sizeof(int32_t));
    }
    //prev_x
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->shared_state->prev_x[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE), 0); //input data is 1.31 so initialising with exp -31
        available_mem_start += ((AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE)*sizeof(int32_t));
    }
    
    //H_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        for(unsigned ph=0; ph<(num_x_channels * num_phases); ph++) {
            bfp_complex_s32_init(&state->H_hat[ch][ph], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
            available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
        }
    }
    //X_fifo
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        for(unsigned ph=0; ph<num_phases; ph++) {
            bfp_complex_s32_init(&state->shared_state->X_fifo[ch][ph], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
            available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
        }
    }
    //initialise Error
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Error[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }
    //Initiaise Y_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Y_hat[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }

    //X_energy 
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0); 
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }
    //sigma_XX
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->shared_state->sigma_XX[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }
    //inv_X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->inv_X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }

    //overlap
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->overlap[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, 32, 0);
        available_mem_start += (32*sizeof(int32_t)); 
    }
    uint32_t memory_used = available_mem_start - (uint8_t*)mem_pool; 
    memset(mem_pool, 0, memory_used);

    //Initialise ema energy
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        state->shared_state->y_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
        state->error_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
    }
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        state->shared_state->x_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
    }
    //fractional regularisation scalefactor
    state->delta_scale = f64_to_float_s32((double)1e-5);

    //Initialise aec config params
    aec_priv_init_config_params(&state->shared_state->config_params);

    //Initialise coherence mu params
    coherence_mu_params_t *coh_params = state->shared_state->coh_mu_state;
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        coh_params[ch].coh = f64_to_float_s32(1.0);
        coh_params[ch].coh_slow = f64_to_float_s32(0.0);
        coh_params[ch].mu_coh_count = 0;
        coh_params[ch].mu_shad_count = 0;
    }

    //Initialise shadow filter params
    shadow_filter_params_t *shadow_params = &state->shared_state->shadow_filter_params;
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        shadow_params->shadow_flag[ch] = EQUAL;
        shadow_params->shadow_reset_count[ch] = -(state->shared_state->config_params.shadow_filt_conf.shadow_reset_timer);
        shadow_params->shadow_better_count[ch] = 0;
    }
}

void aec_priv_shadow_init(
        aec_state_t *state,
        aec_shared_state_t *shared_state,
        uint8_t *mem_pool,
        unsigned num_phases)
{
    if(state == NULL) {
        return;
    }
    memset(state, 0, sizeof(aec_state_t));
    uint8_t *available_mem_start = (uint8_t*)mem_pool;
    
    //initialise number of phases
    state->num_phases = num_phases;

    state->shared_state = shared_state;
    unsigned num_y_channels = state->shared_state->num_y_channels;
    unsigned num_x_channels = state->shared_state->num_x_channels;

    //H_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        for(unsigned ph=0; ph<(num_x_channels * num_phases); ph++) {
            bfp_complex_s32_init(&state->H_hat[ch][ph], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
            available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
        }
    }
    //initialise Error
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Error[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }
    //Initiaise Y_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Y_hat[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }
    //initialise T
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_complex_s32_init(&state->T[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }

    //X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
       bfp_s32_init(&state->X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0); 
       available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }
    //inv_X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->inv_X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }

    //overlap
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->overlap[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, 32, 0);
        available_mem_start += (32*sizeof(int32_t)); 
    }

    uint32_t memory_used = available_mem_start - (uint8_t*)mem_pool; 
    memset(mem_pool, 0, memory_used);

    //Initialise ema energy
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        state->error_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
    }
    //fractional regularisation scalefactor
    state->delta_scale = f64_to_float_s32((double)1e-3);
}

void aec_priv_bfp_complex_s32_copy(
        bfp_complex_s32_t *dst,
        const bfp_complex_s32_t *src)
{
    //This assumes that both dst and src are same length
    memcpy(dst->data, src->data, dst->length*sizeof(complex_s32_t));
    dst->exp = src->exp;
    dst->hr = src->hr;
}

void aec_priv_bfp_s32_reset(bfp_s32_t *a)
{
    memset(a->data, 0, a->length*sizeof(int32_t));
    a->exp = AEC_ZEROVAL_EXP;
    a->hr = AEC_ZEROVAL_HR;
}

void aec_priv_bfp_complex_s32_reset(bfp_complex_s32_t *a)
{
    memset(a->data, 0, a->length*sizeof(complex_s32_t));
    a->exp = AEC_ZEROVAL_EXP;
    a->hr = AEC_ZEROVAL_HR;
}

void aec_priv_reset_filter(
        bfp_complex_s32_t *H_hat,
        unsigned num_x_channels,
        unsigned num_phases)
{
    for(unsigned ph=0; ph<num_x_channels*num_phases; ph++) {
        aec_priv_bfp_complex_s32_reset(&H_hat[ph]);
    }
}

void aec_priv_copy_filter(
        bfp_complex_s32_t *H_hat_dst,
        const bfp_complex_s32_t *H_hat_src,
        unsigned num_x_channels,
        unsigned num_dst_phases,
        unsigned num_src_phases)
{
    uint32_t phases_to_copy = num_src_phases;
    if(num_dst_phases < phases_to_copy) {
        phases_to_copy = num_dst_phases;
    }
    //Copy the H_hat_src phases into H_hat_dst
    for(uint32_t ch=0; ch<num_x_channels; ch++) {
        uint32_t dst_ph_start_offset = ch * num_dst_phases;
        uint32_t src_ph_start_offset = ch * num_src_phases;
        for(uint32_t ph=0; ph<phases_to_copy; ph++) {
            aec_priv_bfp_complex_s32_copy(&H_hat_dst[dst_ph_start_offset + ph], &H_hat_src[src_ph_start_offset + ph]);
        }
    }
    //Zero the remaining H_hat_dst phases
    for(uint32_t ch=0; ch<num_x_channels; ch++) {
        uint32_t dst_ph_start_offset = ch * num_dst_phases;
        for(uint32_t ph=num_src_phases; ph<num_dst_phases; ph++) {
            aec_priv_bfp_complex_s32_reset(&H_hat_dst[dst_ph_start_offset + ph]);
        }
    }
}

void aec_priv_compare_filters(
        aec_state_t *main_state,
        aec_state_t *shadow_state)
{
    aec_shared_state_t *shared_state = main_state->shared_state;
    shadow_filt_config_params_t *shadow_conf = &shared_state->config_params.shadow_filt_conf;
    shadow_filter_params_t *shadow_params = &shared_state->shadow_filter_params;
    unsigned ref_low_all_xch = 1;
    for(unsigned ch=0; ch<main_state->shared_state->num_x_channels; ch++) {
        if(float_s32_gte(shared_state->sum_X_energy[ch], shadow_conf->x_energy_thresh)) {
            ref_low_all_xch = 0;
            break;
        }
    }
    for(unsigned ch=0; ch<main_state->shared_state->num_y_channels; ch++) {
        main_state->shared_state->overall_Y[ch].exp -= 1; //Y_data is 512 samples, Errors are 272 (inc window), approx half the size
        //printf("Ov_Error_shad = %f, Ov_Error = %f, Ov_input = %f\n", float_s32_to_double(shadow_state->overall_Error[ch]), float_s32_to_double(main_state->overall_Error[ch]), float_s32_to_double(shared_state->overall_Y[ch]));
        float_s32_t shadow_copy_thresh_x_Ov_Error = float_s32_mul(shadow_conf->shadow_copy_thresh, main_state->overall_Error[ch]);
        float_s32_t shadow_sigma_thresh_x_Ov_Error = float_s32_mul(shadow_conf->shadow_sigma_thresh, main_state->overall_Error[ch]);
        float_s32_t shadow_reset_thresh_x_Ov_Error = float_s32_mul(shadow_conf->shadow_reset_thresh, main_state->overall_Error[ch]);
        //# check if shadow or reference filter will be used and flag accordingly
        if(ref_low_all_xch) {
            //# input level is low, so error is unreliable, do nothing
            shadow_params->shadow_flag[ch] = LOW_REF;
            continue;
        }
        //# if error way bigger than input, reset- should percolate through to main filter if better
        if(float_s32_gt(shadow_state->overall_Error[ch], shared_state->overall_Y[ch]) && shadow_params->shadow_reset_count[ch] >= 0)
        {
            shadow_params->shadow_flag[ch] = ERROR;
            aec_priv_reset_filter(shadow_state->H_hat[ch], shadow_state->shared_state->num_x_channels, shadow_state->num_phases);
            //Y -> shadow Error
            aec_priv_bfp_complex_s32_copy(&shadow_state->Error[ch], &shared_state->Y[ch]);
            shadow_state->overall_Error[ch] = shared_state->overall_Y[ch];
            //# give the zeroed filter time to reconverge (or redeconverge)
            shadow_params->shadow_reset_count[ch] = -(int)shadow_conf->shadow_reset_timer;
        }
        if(float_s32_gte(shadow_copy_thresh_x_Ov_Error, shadow_state->overall_Error[ch]) &&
                (shadow_params->shadow_better_count[ch] > shadow_conf->shadow_better_thresh)) {
            //# if shadow filter is much better, and has been for several frames,
            //# copy to reference filter
            shadow_params->shadow_flag[ch] = COPY;
            shadow_params->shadow_reset_count[ch] = 0;
            shadow_params->shadow_better_count[ch] += 1;
            //shadow Error -> Error
            aec_priv_bfp_complex_s32_copy(&main_state->Error[ch], &shadow_state->Error[ch]);
            //shadow filter -> main filter
            aec_priv_copy_filter(main_state->H_hat[ch], shadow_state->H_hat[ch], main_state->shared_state->num_x_channels, main_state->num_phases, shadow_state->num_phases);
        }
        else if(float_s32_gte(shadow_sigma_thresh_x_Ov_Error, shadow_state->overall_Error[ch]))
        {
            shadow_params->shadow_better_count[ch] += 1;
            if(shadow_params->shadow_better_count[ch] > shadow_conf->shadow_better_thresh) {
                //# if shadow is somewhat better, reset sigma_xx if both channels are better
                shadow_params->shadow_flag[ch] = SIGMA;
            }
            else {
                shadow_params->shadow_flag[ch] = EQUAL;
            }
        }
        else if(float_s32_gte(shadow_state->overall_Error[ch], shadow_reset_thresh_x_Ov_Error) && 
                shadow_params->shadow_reset_count[ch] >= 0)
        {
            //# if shadow filter is worse than reference, reset provided that
            //# the delay is small and we're not letting the shadow filter reconverge after zeroing
            shadow_params->shadow_reset_count[ch] += 1;
            shadow_params->shadow_better_count[ch] = 0;
            if(shadow_params->shadow_reset_count[ch] > shadow_conf->shadow_zero_thresh) {
                //# if shadow filter has been reset several times in a row, reset to zeros
                shadow_params->shadow_flag[ch] = ZERO;
                aec_priv_reset_filter(shadow_state->H_hat[ch], shadow_state->shared_state->num_x_channels, shadow_state->num_phases);
                aec_priv_bfp_complex_s32_copy(&shadow_state->Error[ch], &shared_state->Y[ch]);
                //# give the zeroed filter time to reconverge (or redeconverge)
                shadow_params->shadow_reset_count[ch] = -(int)shadow_conf->shadow_reset_timer;
            }
            else {
                //debug_printf("Frame %d, main -> shadow filter copy.\n",frame_counter);
                //# otherwise copy the main filter to the shadow filter
                aec_priv_copy_filter(shadow_state->H_hat[ch], main_state->H_hat[ch], main_state->shared_state->num_x_channels, shadow_state->num_phases, main_state->num_phases);
                aec_priv_bfp_complex_s32_copy(&shadow_state->Error[ch], &main_state->Error[ch]);
                shadow_params->shadow_flag[ch] = RESET;
            }
        }
        else {
            //# shadow filter is comparable to main filter, 
            //# or we're waiting for it to reconverge after zeroing
            shadow_params->shadow_better_count[ch] = 0;
            shadow_params->shadow_flag[ch] = EQUAL;
            if(shadow_params->shadow_reset_count[ch] < 0) {
                shadow_params->shadow_reset_count[ch] += 1;
            }
        }
    }
    unsigned all_channels_shadow = 1;
    for(unsigned ch=0; ch<main_state->shared_state->num_y_channels; ch++) {
        if(shadow_params->shadow_flag[ch] <= EQUAL) {
            all_channels_shadow = 0;
        }
    }

    if(all_channels_shadow) {
        for(unsigned ch=0; ch<main_state->shared_state->num_x_channels; ch++)
        {
            aec_priv_bfp_s32_reset(&shared_state->sigma_XX[ch]);
        }
    }
}

void aec_priv_calc_coherence_mu(
        coherence_mu_params_t *coh_mu_state,
        const coherence_mu_config_params_t *coh_conf,
        const float_s32_t *sum_X_energy,
        const int32_t *shadow_flag,
        unsigned num_y_channels,
        unsigned num_x_channels)
{
    //# If the coherence has been low within the last 15 frames, keep the count != 0
    for(unsigned ch=0; ch<num_y_channels; ch++)
    {
        if(coh_mu_state[ch].mu_coh_count > 0) {
            coh_mu_state[ch].mu_coh_count += 1;
        }
        if(coh_mu_state[ch].mu_coh_count > coh_conf->mu_coh_time) {
            coh_mu_state[ch].mu_coh_count = 0;
        }
    }
    //# If the shadow filter has be en used within the last 15 frames, keep the count != 0
    for(unsigned ch=0; ch<num_y_channels; ch++)
    {
        if(shadow_flag[ch] == COPY) {
            coh_mu_state[ch].mu_shad_count = 1;
        }
        else if(coh_mu_state[ch].mu_shad_count > 0) {
            coh_mu_state[ch].mu_shad_count += 1;
        }
        if(coh_mu_state[ch].mu_shad_count > coh_conf->mu_shad_time) {
            coh_mu_state[ch].mu_shad_count = 0;
        }
    }
    //# threshold for coherence between y and y_hat
    float_s32_t min_coh_slow = coh_mu_state[0].coh_slow;
    for(unsigned ch=1; ch<num_y_channels; ch++)
    {
        if(float_s32_gt(min_coh_slow, coh_mu_state[ch].coh_slow)) {
            min_coh_slow = coh_mu_state[ch].coh_slow;
        }
    }
    //# threshold for coherence between y and y_hat
    float_s32_t CC_thres = float_s32_mul(min_coh_slow, coh_conf->coh_thresh_slow);
    for(unsigned ch=0; ch<num_y_channels; ch++)
    {
        if(shadow_flag[ch] >= SIGMA) {
            //# if the shadow filter has triggered, override any drop in coherence
            coh_mu_state[ch].mu_coh_count = 0;
        }
        else {
            //# otherwise if the coherence is low start the count
            if(float_s32_gt(coh_conf->coh_thresh_abs, coh_mu_state[ch].coh)) {
                coh_mu_state[ch].mu_coh_count = 1;
            }
        }
    }
    if(coh_conf->adaption_config == AEC_ADAPTION_AUTO){
        //# Order of priority for coh_mu:
        //# 1) if the reference energy is low, don't converge (not enough SNR to be accurate)
        //# 2) if shadow filter has triggered recently, converge fast
        //# 3) if coherence has dropped recently, don't converge
        //# 4) otherwise, converge fast.
        for(unsigned ch=0; ch<num_y_channels; ch++) {
            if(coh_mu_state[ch].mu_shad_count >= 1)
            {
                for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                    coh_mu_state[ch].coh_mu[x_ch] = f64_to_float_s32(1.0); //TODO profile f64_to_float_s32
                }
            }
            else if(coh_mu_state[ch].mu_coh_count > 0)
            {
                for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                    coh_mu_state[ch].coh_mu[x_ch] = f64_to_float_s32(0);
                }
            }
            else { //# if yy_hat coherence denotes absence of near-end/noise
                if(float_s32_gt(coh_mu_state[ch].coh, coh_mu_state[ch].coh_slow)) {
                    for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                        coh_mu_state[ch].coh_mu[x_ch] = f64_to_float_s32(1.0);
                    }
                }
                else if(float_s32_gt(coh_mu_state[ch].coh, CC_thres))
                {
                    //# scale coh_mu depending on how far above the threshold it is
                    //self.mu[y_ch] = ((self.coh[y_ch]-CC_thres)/(self.coh_slow[y_ch]-CC_thres))**2
                    float_s32_t s1 = float_s32_sub(coh_mu_state[ch].coh, CC_thres);
                    float_s32_t s2 = float_s32_sub(coh_mu_state[ch].coh_slow, CC_thres);
                    float_s32_t s3 = float_s32_div(s1, s2);
                    s3 = float_s32_mul(s3, s3);
                    for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                        coh_mu_state[ch].coh_mu[x_ch] = s3;
                    }
                }
                else {
                    for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                        coh_mu_state[ch].coh_mu[x_ch] = f64_to_float_s32(0);
                    }
                }
            }
        }
        float_s32_t max_ref_energy = sum_X_energy[0];
        for(unsigned x_ch=1; x_ch<num_x_channels; x_ch++) {
            if(float_s32_gt(sum_X_energy[x_ch], max_ref_energy)) {
                max_ref_energy = sum_X_energy[x_ch];
            }
        }
        //np.max(ref_energy_log)-20 is done as (max_ref_energy_not_log*(pow(10, -20/10)))
        float_s32_t max_ref_energy_minus_20dB = float_s32_mul(max_ref_energy, coh_conf->thresh_minus20dB);
        for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
            //if ref_energy_log[x_ch] <= ref_energy_thresh or ref_energy_log[x_ch] < np.max(ref_energy_log)-20: 
            //        self.mu[:, x_ch] = 0
            if(float_s32_gte(coh_conf->x_energy_thresh, sum_X_energy[x_ch]) ||
                float_s32_gt(max_ref_energy_minus_20dB, sum_X_energy[x_ch])
                )
            {
                for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {
                    coh_mu_state[y_ch].coh_mu[x_ch] = f64_to_float_s32(0);
                }
            }
        }
        for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {
            for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                coh_mu_state[y_ch].coh_mu[x_ch] = float_s32_mul(coh_mu_state[y_ch].coh_mu[x_ch], coh_conf->mu_scalar);
            }
        }
    }
    else if(coh_conf->adaption_config == AEC_ADAPTION_FORCE_ON){
        for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {
            for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                coh_mu_state[y_ch].coh_mu[x_ch].mant = coh_conf->force_adaption_mu_q30;
                const int32_t q30_exp = -30;
                coh_mu_state[y_ch].coh_mu[x_ch].exp = q30_exp;
            }
        }
    }
    else if(coh_conf->adaption_config == AEC_ADAPTION_FORCE_OFF){
        for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {
            for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                coh_mu_state[y_ch].coh_mu[x_ch] = f64_to_float_s32(0);
            }
        }
    }
    /*for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {
      for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
        printf("mu[%d][%d] = %f\n",y_ch, x_ch, float_s32_to_double(coh_mu_state[y_ch].coh_mu[x_ch]));
        
      }
    }*/
}

void aec_priv_bfp_complex_s32_recalc_energy_one_bin(
        bfp_s32_t *X_energy,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *X,
        unsigned num_phases,
        unsigned recalc_bin)
{
    int32_t sum_out_data = 0;
    bfp_s32_t sum_out;
    bfp_s32_init(&sum_out, &sum_out_data, AEC_ZEROVAL_EXP, 1, 0);

    int32_t t=0;
    bfp_s32_t temp_out;
    bfp_s32_init(&temp_out, &t, 0, 1, 0);
    bfp_complex_s32_t temp_in;

    for(unsigned i=0; i<num_phases-1; i++) {
        bfp_complex_s32_init(&temp_in, &X_fifo[i].data[recalc_bin], X_fifo[i].exp, 1, 1);
        bfp_complex_s32_squared_mag(&temp_out, &temp_in);
        bfp_s32_add(&sum_out, &sum_out, &temp_out);
    }
    bfp_complex_s32_init(&temp_in, &X->data[recalc_bin], X->exp, 1, 1);
    
    bfp_complex_s32_squared_mag(&temp_out, &temp_in);
    bfp_s32_add(&sum_out, &sum_out, &temp_out);
    bfp_s32_use_exponent(&sum_out, X_energy->exp);

    // manage headroom mismatch
    X_energy->data[recalc_bin] = sum_out.data[0];
    if(sum_out.hr < X_energy->hr) {
        X_energy->hr = sum_out.hr;
    }
    //printf("after recalc 0x%lx\n",X_energy->data[recalc_bin]);
}

void aec_priv_update_total_X_energy(
        bfp_s32_t *X_energy,
        float_s32_t *max_X_energy,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *X,
        unsigned num_phases,
        unsigned recalc_bin)
{
    int32_t DWORD_ALIGNED energy_scratch[AEC_PROC_FRAME_LENGTH/2 + 1];
    bfp_s32_t scratch;
    bfp_s32_init(&scratch, energy_scratch, 0, AEC_PROC_FRAME_LENGTH/2+1, 0);
    //X_fifo ordered from newest to oldest phase
    //subtract oldest phase
    bfp_complex_s32_squared_mag(&scratch, &X_fifo[num_phases-1]);
    bfp_s32_sub(X_energy, X_energy, &scratch);
    //add newest phase
    bfp_complex_s32_squared_mag(&scratch, X);
    bfp_s32_add(X_energy, X_energy, &scratch);

    aec_priv_bfp_complex_s32_recalc_energy_one_bin(X_energy, X_fifo, X, num_phases, recalc_bin);

    /** Due to fixed point arithmetic we might sometimes see -ve numbers in X_energy, so clamp to a minimum of 0. This
     * happens if all the energy in X_energy is made of the phase being subtracted out and the new X data energy being
     * added is 0*/
    bfp_s32_rect(X_energy, X_energy);

    *max_X_energy = bfp_s32_max(X_energy);
    /** Steps taken to make sure divide by 0 doesn't happen while calculating inv_X_energy in aec_priv_calc_inverse().
      * Divide by zero Scenario 1: All X_energy bins are 0 => max_X_energy is 0, but the exponent is something reasonably big, like
      * -34 and delta value ends up as delta min which is (some_non_zero_mant, -97 exp). So we end up with inv_X_energy
      * = 1/denom, where denom is (zero_mant, -34 exp) + (some_non_zero_mant, -97 exp) which is still calculated as 0
      * mant. To avoid this situation, we set X_energy->exp to something much smaller (like AEC_ZEROVAL_EXP) than delta_min->exp so that
      * (zero_mant, AEC_ZEROVAL_EXP exp) + (some_non_zero_mant, -97 exp) = (some_non_zero_mant, -97 exp). I haven't been able to
      * recreate this situation.
      *
      * Divide by zero Scenario 2: A few X_energy bins are 0 with exp something reasonably big and delta is delta_min.
      * We'll not be able to find this happen by checking for max_X_energy->mant == 0. I have addressed this in
      * aec_priv_calc_inv_X_energy_denom()
      */

    //Scenario 1 (All bins 0 mant) fix
    if(max_X_energy->mant == 0) {
        X_energy->exp = AEC_ZEROVAL_EXP;
    }    
}

void aec_priv_update_X_fifo_and_calc_sigmaXX(
        bfp_complex_s32_t *X_fifo,
        bfp_s32_t *sigma_XX,
        float_s32_t *sum_X_energy,
        const bfp_complex_s32_t *X,
        unsigned num_phases,
        uint32_t sigma_xx_shift)
{
    /* Note: Instead of maintaining a separate mapping array, I'll instead, shift around the X_fifo array at the end of update_X_fifo.
    * This will only involve memcpys of size x_channels*num_phases*sizeof(bfp_complex_s32_t).
    */
    //X-fifo update
    //rearrage X-fifo to point from newest phase to oldest phase
    bfp_complex_s32_t last_phase = X_fifo[num_phases-1];
    for(int32_t n=num_phases-1; n>=1; n--) {
        X_fifo[n] = X_fifo[n-1];
    }
    X_fifo[0] = last_phase;
    //Update X as newest phase
    memcpy(X_fifo[0].data, X->data, X->length*sizeof(complex_s32_t));
    X_fifo[0].exp = X->exp;
    X_fifo[0].hr = X->hr;
    X_fifo[0].length = X->length;
    
    //update sigma_XX
    int32_t DWORD_ALIGNED sigma_scratch_mem[AEC_PROC_FRAME_LENGTH/2 + 1];
    bfp_s32_t scratch;
    bfp_s32_init(&scratch, sigma_scratch_mem, 0, AEC_FD_FRAME_LENGTH, 0);
    bfp_complex_s32_squared_mag(&scratch, X);
    float_s64_t sum = bfp_s32_sum(&scratch);
    *sum_X_energy = float_s64_to_float_s32(sum);

    //(pow(2, -ema_coef_shr))*X_energy
    scratch.exp -= sigma_xx_shift;

    //sigma_XX * (1 - pow(2, -ema_coef_shr)) = sigma_XX - (sigma_XX * pow(2, -ema_coef_shr))
    bfp_s32_t sigma_XX_scaled = *sigma_XX;
    sigma_XX_scaled.exp -= sigma_xx_shift;
    bfp_s32_sub(sigma_XX, sigma_XX, &sigma_XX_scaled); //sigma_XX - (sigma_XX * pow(2, -ema_coef_shr))
    bfp_s32_add(sigma_XX, sigma_XX, &scratch); //sigma_XX - (sigma_XX * pow(2, -ema_coef_shr)) + (pow(2, -ema_coef_shr))*X_energy

}

void aec_priv_calc_Error_and_Y_hat(
        bfp_complex_s32_t *Error,
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *Y,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *H_hat,
        unsigned num_x_channels,
        unsigned num_phases,
        int32_t bypass_enabled)
{
    aec_l2_calc_Error_and_Y_hat(Error, Y_hat, Y, X_fifo, H_hat, num_x_channels, num_phases, 0, AEC_PROC_FRAME_LENGTH/2 + 1, bypass_enabled);
}

void aec_priv_calc_coherence(
        coherence_mu_params_t *coh_mu_state,
        const bfp_s32_t *y_subset,
        const bfp_s32_t *y_hat_subset,
        const aec_config_params_t *conf)
{
    const coherence_mu_config_params_t *coh_conf = &conf->coh_mu_conf;

    float_s32_t sigma_yy = float_s64_to_float_s32(bfp_s32_dot(y_subset, y_subset));
    float_s32_t sigma_yhatyhat = float_s64_to_float_s32(bfp_s32_dot(y_hat_subset, y_hat_subset));
    float_s32_t sigma_yyhat = float_s64_to_float_s32(bfp_s32_dot(y_subset, y_hat_subset));

    //# Calculate coherence between y and y_hat
    //eps = 1e-100
    //this_coh = np.abs(sigma_yyhat/(np.sqrt(sigma_yy)*np.sqrt(sigma_yhatyhat) + eps))
    float_s32_t denom;
    denom = float_s32_mul(sigma_yy, sigma_yhatyhat);
    denom = float_s32_sqrt(denom);
    denom = float_s32_add(denom, coh_conf->eps);
    if(denom.mant == 0) {
        denom = coh_conf->eps;
    }

    float_s32_t this_coh = float_s32_div(float_s32_abs(sigma_yyhat), denom);

    //# moving average coherence
    //self.coh = self.coh_alpha*self.coh + (1.0 - self.coh_alpha)*this_coh
    float_s32_t one = f64_to_float_s32(1.0); //TODO profile this call
    float_s32_t one_minus_alpha = float_s32_sub(one, coh_conf->coh_alpha);
    float_s32_t t1 = float_s32_mul(coh_conf->coh_alpha, coh_mu_state->coh);
    float_s32_t t2 = float_s32_mul(one_minus_alpha, this_coh);
    coh_mu_state->coh = float_s32_add(t1, t2);

    //# update slow moving averages used for thresholding
    //self.coh_slow = self.coh_slow_alpha*self.coh_slow + (1.0 - self.coh_slow_alpha)*self.coh
    float_s32_t one_minus_slow_alpha = float_s32_sub(one, coh_conf->coh_slow_alpha);
    t1 = float_s32_mul(coh_conf->coh_slow_alpha, coh_mu_state->coh_slow);
    t2 = float_s32_mul(one_minus_slow_alpha, coh_mu_state->coh);
    coh_mu_state->coh_slow = float_s32_add(t1, t2);
}

float_s32_t aec_priv_calc_corr_factor(bfp_s32_t *y, bfp_s32_t *yhat) {
    // abs(sigma_yyhat)/(sigma_abs(y)abs(yhat))
    int32_t DWORD_ALIGNED y_abs_mem[AEC_FRAME_ADVANCE]; 
    int32_t DWORD_ALIGNED yhat_abs_mem[AEC_FRAME_ADVANCE]; 
    bfp_s32_t y_abs, yhat_abs;

    bfp_s32_init(&y_abs, &y_abs_mem[0], 0, y->length, 0);
    bfp_s32_init(&yhat_abs, &yhat_abs_mem[0], 0, yhat->length, 0);

    bfp_s32_abs(&y_abs, y);
    bfp_s32_abs(&yhat_abs, yhat);

    float_s32_t num, denom;
    // sigma_yyhat
    num = float_s64_to_float_s32(bfp_s32_dot(y, yhat));
    // sigma_abs(y)abs(yhat)
    denom = float_s64_to_float_s32(bfp_s32_dot(&y_abs, &yhat_abs));
    
    // abs(sigma_yyhat)/sigma_abs(y)abs(yhat)
    if(denom.mant == 0) {
        /** denom 0 implies sigma_abs(y)abs(yhat) is 0 which in turn means y or y_hat is 0. y 0 means no near end, y_hat
         * 0 means no far end and for both these, we don't want AGC LC to apply extra attenuation so setting corr_factor
         * to 0*/
        return (float_s32_t){0, -31};
    }
    float_s32_t corr_factor = float_s32_div(float_s32_abs(num), denom);

    return corr_factor;
}

// Hanning window structure used in the windowing operation done to remove discontinuities from the filter error
static const uq1_31 WOLA_window_q31[AEC_UNUSED_TAPS_PER_PHASE*2] = {
       4861986,   19403913,   43494088,   76914346,  119362028,  170452721,  229723740,  296638317,
     370590464,  450910459,  536870911,  627693349,  722555272,  820597594,  920932429, 1022651130,
    1124832516, 1226551217, 1326886052, 1424928374, 1519790297, 1610612735, 1696573187, 1776893182,
    1850845329, 1917759906, 1977030925, 2028121618, 2070569300, 2103989558, 2128079733, 2142621660
};

static const uq1_31 WOLA_window_flpd_q31[AEC_UNUSED_TAPS_PER_PHASE*2] = {
    2142621660, 2128079733, 2103989558, 2070569300, 2028121618, 1977030925, 1917759906, 1850845329, 
    1776893182, 1696573187, 1610612735, 1519790297, 1424928374, 1326886052, 1226551217, 1124832516, 
    1022651130, 920932429, 820597594, 722555272, 627693349, 536870911, 450910459, 370590464, 
    296638317, 229723740, 170452721, 119362028, 76914346, 43494088, 19403913, 4861986, 
};

void aec_priv_create_output(
        bfp_s32_t *output,
        bfp_s32_t *overlap,
        bfp_s32_t *error)
{
    bfp_s32_t win, win_flpd;
    bfp_s32_init(&win, (int32_t*)&WOLA_window_q31[0], AEC_WINDOW_EXP, (AEC_UNUSED_TAPS_PER_PHASE*2), 0);
    bfp_s32_init(&win_flpd, (int32_t*)&WOLA_window_flpd_q31[0], AEC_WINDOW_EXP, (AEC_UNUSED_TAPS_PER_PHASE*2) , 0);

    //zero first 240 samples
    memset(error->data, 0, AEC_FRAME_ADVANCE*sizeof(int32_t));

    bfp_s32_t chunks[2];
    bfp_s32_init(&chunks[0], &error->data[AEC_FRAME_ADVANCE], error->exp, AEC_UNUSED_TAPS_PER_PHASE*2, 1); //240-272 fwd win
    bfp_s32_init(&chunks[1], &error->data[2*AEC_FRAME_ADVANCE], error->exp, AEC_UNUSED_TAPS_PER_PHASE*2, 1); //480-512 flpd win

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
        memcpy(output->data, &error->data[AEC_FRAME_ADVANCE], AEC_FRAME_ADVANCE*sizeof(int32_t));
        output->length = AEC_FRAME_ADVANCE;
        output->exp = error->exp;
        output->hr = error->hr;

        //overlap add
        //split output into 2 chunks. chunk[0] with first 32 samples of output. chunk[1] has rest of the 240-32 samples of output
        bfp_s32_init(&chunks[0], &output->data[0], output->exp, AEC_UNUSED_TAPS_PER_PHASE*2, 1);
        bfp_s32_init(&chunks[1], &output->data[AEC_UNUSED_TAPS_PER_PHASE*2], output->exp, AEC_FRAME_ADVANCE-(AEC_UNUSED_TAPS_PER_PHASE*2), 1);

        //Add previous frame's overlap to first 32 samples of output
        bfp_s32_add(&chunks[0], &chunks[0], overlap);
        bfp_s32_use_exponent(&chunks[0], AEC_INPUT_EXP); //bring the overlapped-added part back to 1.31 since output is same format as input
        bfp_s32_use_exponent(&chunks[1], AEC_INPUT_EXP); //bring the rest of output to 1.31 since output is same format as input
        output->hr = (chunks[0].hr < chunks[1].hr) ? chunks[0].hr : chunks[1].hr;
    }
    
    //update overlap
    memcpy(overlap->data, &error->data[2*AEC_FRAME_ADVANCE], (AEC_UNUSED_TAPS_PER_PHASE*2)*sizeof(int32_t));
    overlap->hr = error->hr;
    overlap->exp = error->exp;
}
extern void vtb_inv_X_energy_asm(uint32_t *inv_X_energy,
        unsigned shr,
        unsigned count);

void aec_priv_calc_inverse(
        bfp_s32_t *input)
{
#if 1
    //82204 cycles. 2 x-channels, single thread, but get rids of voice_toolbox dependency on vtb_inv_X_energy_asm (36323 cycles)
    bfp_s32_inverse(input, input);

#else //36323 cycles. 2 x-channels, single thread
    int32_t min_element = vect_s32_min(
                                input->data,
                                input->length);
 
    // HR_S32() gets headroom of a single int32_t
    //old aec would calculate shr as HR_S32(min_element) + 2. Since VPU deals with only signed numbers, increase shr by 1 to account for sign bit in the result of the inverse function.
    int input_shr = HR_S32(min_element) + 2 + 1;
    //vtb_inv_X_energy
    input->exp = (-input->exp - 32); //TODO work out this mysterious calculation
    input->exp -= (32 - input_shr);
    vtb_inv_X_energy_asm((uint32_t *)input->data, input_shr, input->length);
    input->hr = 0;
#endif
}


void bfp_new_add_scalar(
    bfp_s32_t* a, 
    const bfp_s32_t* b, 
    const float_s32_t c)
{
#if (BFP_DEBUG_CHECK_LENGTHS)
    assert(b->length == a->length);
    assert(b->length != 0);
#endif

    right_shift_t b_shr, c_shr;

    vect_s32_add_scalar_prepare(&a->exp, &b_shr, &c_shr, b->exp, c.exp, 
                                    b->hr, HR_S32(c.mant));

    int32_t cc = 0;
    if (c_shr < 32)
        cc = (c_shr >= 0)? (c.mant >> c_shr) : (c.mant << -c_shr);

    a->hr = vect_s32_add_scalar(a->data, b->data, cc, b->length, 
                                    b_shr);
}

void aec_priv_calc_inv_X_energy_denom(
        bfp_s32_t *inv_X_energy_denom,
        const bfp_s32_t *X_energy,
        const bfp_s32_t *sigma_XX,
        const aec_config_params_t *conf,
        float_s32_t delta,
        unsigned is_shadow,
        unsigned normdenom_apply_factor_of_2) {
    
    int gamma_log2 = conf->aec_core_conf.gamma_log2;
    if(!is_shadow) { //frequency smoothing
        int32_t norm_denom_buf[AEC_PROC_FRAME_LENGTH/2 + 1];
        bfp_s32_t norm_denom;
        bfp_s32_init(&norm_denom, &norm_denom_buf[0], 0, AEC_PROC_FRAME_LENGTH/2+1, 0);

        bfp_s32_t sigma_times_gamma;
        bfp_s32_init(&sigma_times_gamma, sigma_XX->data, sigma_XX->exp+gamma_log2, sigma_XX->length, 0);
        sigma_times_gamma.hr = sigma_XX->hr;
        //TODO 3610 AEC calculates norm_denom as normDenom = 2*self.X_energy[:,k] + self.sigma_xx*gamma 
        //instead of normDenom = self.X_energy[:,k] + self.sigma_xx*gamma and ADEC tests pass only with the former.
        bfp_s32_t temp = *X_energy;
        if(normdenom_apply_factor_of_2) {
            temp.exp = temp.exp+1;
        }
        bfp_s32_add(&norm_denom, &sigma_times_gamma, &temp);

        //self.taps = [0.5, 1, 1, 1, 0.5] 
        uq2_30 taps_q30[5] = {0x20000000, 0x40000000, 0x40000000, 0x40000000, 0x20000000};
        for(int i=0; i<5; i++) {
            taps_q30[i] = taps_q30[i] >> 2;//This is equivalent to a divide by 4
        }

        bfp_s32_convolve_same(inv_X_energy_denom, &norm_denom, (const int32_t *) &taps_q30[0], 5, PAD_MODE_REFLECT);

        //bfp_s32_add_scalar(inv_X_energy_denom, inv_X_energy_denom, delta);
        bfp_new_add_scalar(inv_X_energy_denom, inv_X_energy_denom, delta);

    }
    else
    {
        //TODO maybe fix this for AEC?
        // int32_t temp[AEC_PROC_FRAME_LENGTH/2 + 1];
        // bfp_s32_t temp_bfp;
        // bfp_s32_init(&temp_bfp, &temp[0], 0, AEC_PROC_FRAME_LENGTH/2+1, 0);

        // bfp_complex_s32_real_scale(&temp, sigma_XX, gamma)

        bfp_s32_add_scalar(inv_X_energy_denom, X_energy, delta);
    }

    /**Fix for divide by 0 scenario 2 discussed in a comment in aec_priv_update_total_X_energy()
     * We have 2 options.
     * Option 1: Clamp the denom values between max:(denom_max mant, denom->exp exp) and min (1 mant, denom->exp exp).
     * This will change all the (0, exp) bins to (1, exp) while leaving other bins unchanged. This could be done without
     * checking if (bfp_s32_min(denom))->mant is 0, since if there are no zero bins, the bfp_s32_clamp() would change
     * nothing in the denom vector.
     * Option 2: Add a (1 mant, denom->exp) scalar to the denom vector. I'd do this after checking if
     * (bfp_s32_min(denom))->mant is 0 to avoid adding an offset to the denom vector unnecessarily.
     * Since this is not a recreatable scenario I'm not sure which option is better. Going with option 2 since it
     * consumes fewer cycles.
     */
     //Option 1 (3220 cycles)
     /*float_s32_t max = bfp_s32_max(inv_X_energy_denom);
     bfp_s32_clip(inv_X_energy_denom, inv_X_energy_denom, 1, max.mant, inv_X_energy_denom->exp);*/

     //Option 2 (1528 cycles for the bfp_s32_min() call. Haven't profiled when min.mant == 0 is true
     float_s32_t min = bfp_s32_min(inv_X_energy_denom);

     if(min.mant == 0) {
         /** The presence of delta even when it's zero in bfp_s32_add_scalar(inv_X_energy_denom, X_energy, delta); above
          * ensures that bfp_s32_max(inv_X_energy_denom) always has a headroom of 1, making sure that t is not right shifted as part
          * of bfp_s32_add_scalar() making t.mant 0*/
         float_s32_t t = {1, inv_X_energy_denom->exp};
         bfp_s32_add_scalar(inv_X_energy_denom, inv_X_energy_denom, t);
     }
}

// For aec, to get adec tests passing norm_denom in aec_priv_calc_inv_X_energy_denom is calculated as
// normDenom = 2*self.X_energy[:,k] + self.sigma_xx*gamma while in IC its done as
// normDenom = self.X_energy[:,k] + self.sigma_xx*gamma. To work around this, an extra argument normdenom_apply_factor_of_2
// is added to aec_priv_calc_inv_X_energy. When set to 1, X_energy is multiplied by 2 in the inverse energy calculation.
void aec_priv_calc_inv_X_energy(
        bfp_s32_t *inv_X_energy,
        const bfp_s32_t *X_energy,
        const bfp_s32_t *sigma_XX,
        const aec_config_params_t *conf,
        float_s32_t delta,
        unsigned is_shadow,
        unsigned normdenom_apply_factor_of_2)
{
    // Calculate denom for the inv_X_energy = 1/denom calculation. denom calculation is different for shadow and main filter
    aec_priv_calc_inv_X_energy_denom(inv_X_energy, X_energy, sigma_XX, conf, delta, is_shadow, normdenom_apply_factor_of_2);
    aec_priv_calc_inverse(inv_X_energy);
}

void aec_priv_filter_adapt(
        bfp_complex_s32_t *H_hat,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *T,
        unsigned num_x_channels,
        unsigned num_phases)
{
    unsigned phases = num_x_channels * num_phases;
    for(unsigned ph=0; ph<phases; ph++) {
        //find out which channel this phase belongs to
        aec_l2_adapt_plus_fft_gc(&H_hat[ph], &X_fifo[ph], &T[ph/num_phases]);
    }
}

void aec_priv_compute_T(
        bfp_complex_s32_t *T,
        const bfp_complex_s32_t *Error,
        const bfp_s32_t *inv_X_energy,
        float_s32_t mu)
{
    //T[x_ch] = self.mu[y_ch, x_ch] * Inv_x_energy[x_ch] * Error[y_ch] / self.K

    //The more optimal way to calculate T is temp = mu*inv_X_energy followed by T = Error*temp since
    //this will require half the multiplies in mu*inv_X_energy stage, but this will require a temporary buffer
    //of bfp_s32_t type, with length the same as inv_X_energy. So instead, I've done T = inv_X_energy * Error
    //followed by T = T * mu
    bfp_complex_s32_real_mul(T, Error, inv_X_energy);
    bfp_complex_s32_real_scale(T, T, mu);

    //bfp_complex_s32_real_scale(T, Error, mu);
    //bfp_complex_s32_real_mul(T, T, inv_X_energy);
}

void aec_priv_init_config_params(
        aec_config_params_t *config_params)
{
    //TODO profile f64_to_float_s32() calls
    //aec_core_config_params_t
    aec_core_config_params_t *core_conf = &config_params->aec_core_conf;
    core_conf->sigma_xx_shift = 6;
    core_conf->ema_alpha_q30 = Q30(0.98);
    core_conf->gamma_log2 = 5;
    core_conf->delta_adaption_force_on.mant = (unsigned)UINT_MAX >> 1;
    core_conf->delta_adaption_force_on.exp = -32 - 6 + 1; //extra +1 to account for shr of 1 to the mant in order to store it as a signed number
    core_conf->delta_min = f64_to_float_s32((double)1e-20);
    core_conf->bypass = 0;
    core_conf->coeff_index = 0;

    //shadow_filt_config_params_t
    shadow_filt_config_params_t *shadow_cfg = &config_params->shadow_filt_conf;
    shadow_cfg->shadow_sigma_thresh = f64_to_float_s32(0.6); //# threshold for resetting sigma_xx
    shadow_cfg->shadow_copy_thresh = f64_to_float_s32(0.5); //# threshold for copying shadow filter
    shadow_cfg->shadow_reset_thresh = f64_to_float_s32(1.5);
    shadow_cfg->shadow_delay_thresh = f64_to_float_s32(0.5); //# will not reset if reference delay is large
    shadow_cfg->x_energy_thresh = f64_to_float_s32(pow(10, -40/10.0));
    shadow_cfg->shadow_better_thresh = 5; //# how many times better before copying
    shadow_cfg->shadow_zero_thresh = 5;//# zero shadow filter every n resets
    shadow_cfg->shadow_reset_timer = 20; //# number of frames between zeroing resets
    shadow_cfg->shadow_mu = f64_to_float_s32(1.0);

    //coherence_mu_config_params_t 
    coherence_mu_config_params_t *coh_cfg = &config_params->coh_mu_conf;
    coh_cfg->coh_alpha = f64_to_float_s32(0.0);
    coh_cfg->coh_slow_alpha = f64_to_float_s32(0.99);
    coh_cfg->coh_thresh_slow = f64_to_float_s32(0.9);
    coh_cfg->coh_thresh_abs = f64_to_float_s32(0.65);
    coh_cfg->mu_scalar = f64_to_float_s32(1.0);
    coh_cfg->eps = f64_to_float_s32((double)1e-100);
    coh_cfg->thresh_minus20dB = f64_to_float_s32(pow(10, -20/10.0));
    coh_cfg->x_energy_thresh = f64_to_float_s32(pow(10, -40/10.0));
    coh_cfg->mu_coh_time = 2;
    coh_cfg->mu_shad_time = 5;

    coh_cfg->adaption_config = AEC_ADAPTION_AUTO;
    coh_cfg->force_adaption_mu_q30 = Q30(0.1);
}

void aec_priv_calc_delta(
        float_s32_t *delta, 
        const float_s32_t *max_X_energy,
        aec_config_params_t *conf,
        float_s32_t scale,
        uint32_t channels) {
    if(conf->coh_mu_conf.adaption_config == AEC_ADAPTION_AUTO) {
        float_s32_t delta_min = conf->aec_core_conf.delta_min;
        float_s32_t max = max_X_energy[0];
        for(int i=1; i<channels; i++) {
            max = float_s32_gt(max, max_X_energy[i]) ? max : max_X_energy[i];
        }
        max = float_s32_mul(max, scale);
        *delta = float_s32_gt(max, delta_min) ? max : delta_min;
    }
    else {
        *delta = conf->aec_core_conf.delta_adaption_force_on;
    }
}
