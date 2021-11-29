// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_priv.h"

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
    
    //H_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        for(unsigned ph=0; ph<(num_x_channels * num_phases); ph++) {
            bfp_complex_s32_init(&state->H_hat_1d[ch][ph], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
            available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
        }
    }
    //X_fifo
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        for(unsigned ph=0; ph<num_phases; ph++) {
            bfp_complex_s32_init(&state->shared_state->X_fifo[ch][ph], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
            available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
        }
    }
    //initialise Error
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Error[ch], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
    }
    //Initiaise Y_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Y_hat[ch], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
    }

    //X_energy 
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->X_energy[ch], (int32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0); 
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(int32_t)); 
    }
    //sigma_XX
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->shared_state->sigma_XX[ch], (int32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(int32_t)); 
    }
    //inv_X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->inv_X_energy[ch], (int32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(int32_t)); 
    }

    //Initialise output
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->output[ch], (int32_t*)available_mem_start, 0, AEC_FRAME_ADVANCE, 0);
        available_mem_start += (AEC_FRAME_ADVANCE*sizeof(int32_t)); 
    }
    //overlap
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->overlap[ch], (int32_t*)available_mem_start, -1024, 32, 0);
        available_mem_start += (32*sizeof(int32_t)); 
    }
    int memory_used = available_mem_start - (uint8_t*)mem_pool; 
    memset(mem_pool, 0, memory_used);

    //Initialise ema energy
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        state->shared_state->y_ema_energy[ch].exp = -1024;
        state->error_ema_energy[ch].exp = -1024;
    }
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        state->shared_state->x_ema_energy[ch].exp = -1024;
    }
    //fractional regularisation scalefactor
    state->delta_scale = double_to_float_s32((double)1e-5);

    //Initialise aec config params
    aec_priv_init_config_params(&state->shared_state->config_params);

    //Initialise coherence mu params
    coherence_mu_params_t *coh_params = state->shared_state->coh_mu_state;
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        coh_params[ch].coh = double_to_float_s32(1.0);
        coh_params[ch].coh_slow = double_to_float_s32(0.0);
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
            bfp_complex_s32_init(&state->H_hat_1d[ch][ph], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
            available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
        }
    }
    //initialise Error
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Error[ch], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
    }
    //Initiaise Y_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Y_hat[ch], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
    }
    //initialise T
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_complex_s32_init(&state->T[ch], (complex_s32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(complex_s32_t)); 
    }

    //X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
       bfp_s32_init(&state->X_energy[ch], (int32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0); 
       available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(int32_t)); 
    }
    //inv_X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->inv_X_energy[ch], (int32_t*)available_mem_start, -1024, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
        available_mem_start += ((AEC_PROC_FRAME_LENGTH/2 + 1)*sizeof(int32_t)); 
    }

    //Initialise output
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->output[ch], (int32_t*)available_mem_start, 0, AEC_FRAME_ADVANCE, 0);
        available_mem_start += (AEC_FRAME_ADVANCE*sizeof(int32_t)); 
    }
    //overlap
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->overlap[ch], (int32_t*)available_mem_start, -1024, 32, 0);
        available_mem_start += (32*sizeof(int32_t)); 
    }

    int memory_used = available_mem_start - (uint8_t*)mem_pool; 
    memset(mem_pool, 0, memory_used);

    //Initialise ema energy
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        state->error_ema_energy[ch].exp = -1024;
    }
    //fractional regularisation scalefactor
    state->delta_scale = double_to_float_s32((double)1e-3);
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
    a->exp = -1024;
    a->hr = 31;
}

void aec_priv_bfp_complex_s32_reset(bfp_complex_s32_t *a)
{
    memset(a->data, 0, a->length*sizeof(complex_s32_t));
    a->exp = -1024;
    a->hr = 31;
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
    int32_t phases_to_copy = num_src_phases;
    if(num_dst_phases < phases_to_copy) {
        phases_to_copy = num_dst_phases;
    }
    //Copy the H_hat_src phases into H_hat_dst
    for(int ch=0; ch<num_x_channels; ch++) {
        int dst_ph_start_offset = ch * num_dst_phases;
        int src_ph_start_offset = ch * num_src_phases;
        for(int ph=0; ph<phases_to_copy; ph++) {
            aec_priv_bfp_complex_s32_copy(&H_hat_dst[dst_ph_start_offset + ph], &H_hat_src[src_ph_start_offset + ph]);
        }
    }
    //Zero the remaining H_hat_dst phases
    for(int ch=0; ch<num_x_channels; ch++) {
        int dst_ph_start_offset = ch * num_dst_phases;
        for(int ph=num_src_phases; ph<num_dst_phases; ph++) {
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
            aec_priv_reset_filter(shadow_state->H_hat_1d[ch], shadow_state->shared_state->num_x_channels, shadow_state->num_phases);
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
            aec_priv_copy_filter(main_state->H_hat_1d[ch], shadow_state->H_hat_1d[ch], main_state->shared_state->num_x_channels, main_state->num_phases, shadow_state->num_phases);
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
                aec_priv_reset_filter(shadow_state->H_hat_1d[ch], shadow_state->shared_state->num_x_channels, shadow_state->num_phases);
                aec_priv_bfp_complex_s32_copy(&shadow_state->Error[ch], &shared_state->Y[ch]);
                //# give the zeroed filter time to reconverge (or redeconverge)
                shadow_params->shadow_reset_count[ch] = -(int)shadow_conf->shadow_reset_timer;
            }
            else {
                //debug_printf("Frame %d, main -> shadow filter copy.\n",frame_counter);
                //# otherwise copy the main filter to the shadow filter
                aec_priv_copy_filter(shadow_state->H_hat_1d[ch], main_state->H_hat_1d[ch], main_state->shared_state->num_x_channels, shadow_state->num_phases, main_state->num_phases);
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
                    coh_mu_state[ch].coh_mu[x_ch] = double_to_float_s32(1.0); //TODO profile double_to_float_s32
                }
            }
            else if(coh_mu_state[ch].mu_coh_count > 0)
            {
                for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                    coh_mu_state[ch].coh_mu[x_ch] = double_to_float_s32(0);
                }
            }
            else { //# if yy_hat coherence denotes absence of near-end/noise
                if(float_s32_gt(coh_mu_state[ch].coh, coh_mu_state[ch].coh_slow)) {
                    for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                        coh_mu_state[ch].coh_mu[x_ch] = double_to_float_s32(1.0);
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
                        coh_mu_state[ch].coh_mu[x_ch] = double_to_float_s32(0);
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
                    coh_mu_state[y_ch].coh_mu[x_ch] = double_to_float_s32(0);
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
                coh_mu_state[y_ch].coh_mu[x_ch].exp = -30;
            }
        }
    }
    else if(coh_conf->adaption_config == AEC_ADAPTION_FORCE_OFF){
        for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {
            for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                coh_mu_state[y_ch].coh_mu[x_ch] = double_to_float_s32(0);
            }
        }
    }
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
    bfp_s32_init(&sum_out, &sum_out_data, -1024, 1, 0);

    int32_t t;
    bfp_s32_t temp_out;
    bfp_s32_init(&temp_out, &t, 0, 1, 0);
    bfp_complex_s32_t temp_in;

    for(unsigned i=0; i<num_phases-1; i++) {
        bfp_complex_s32_init(&temp_in, &X_fifo[i].data[recalc_bin], X_fifo[i].exp, 1, 0);
        temp_in.hr = X_fifo[i].hr;
        bfp_complex_s32_squared_mag(&temp_out, &temp_in);
        bfp_s32_add(&sum_out, &sum_out, &temp_out);
    }
    bfp_complex_s32_init(&temp_in, &X->data[recalc_bin], X->exp, 1, 0);
    temp_in.hr = X->hr;
    
    bfp_complex_s32_squared_mag(&temp_out, &temp_in);
    bfp_s32_add(&sum_out, &sum_out, &temp_out);
    bfp_s32_use_exponent(&sum_out, X_energy->exp);

    //TODO manage headroom mismatch
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
    *max_X_energy = bfp_s32_max(X_energy);
    return;
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
    for(int n=num_phases-1; n>=1; n--) {
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
    bfp_s32_init(&scratch, sigma_scratch_mem, 0, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
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

    return;
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
    float_s32_t one = double_to_float_s32(1.0); //TODO profile this call
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

static const int32_t WOLA_window[32] = {
       4861986,   19403913,   43494088,   76914346,  119362028,  170452721,  229723740,  296638317,
     370590464,  450910459,  536870911,  627693349,  722555272,  820597594,  920932429, 1022651130,
    1124832516, 1226551217, 1326886052, 1424928374, 1519790297, 1610612735, 1696573187, 1776893182,
    1850845329, 1917759906, 1977030925, 2028121618, 2070569300, 2103989558, 2128079733, 2142621660
};

static const int32_t WOLA_window_flpd[32] = {
    2142621660, 2128079733, 2103989558, 2070569300, 2028121618, 1977030925, 1917759906, 1850845329, 
    1776893182, 1696573187, 1610612735, 1519790297, 1424928374, 1326886052, 1226551217, 1124832516, 
    1022651130, 920932429, 820597594, 722555272, 627693349, 536870911, 450910459, 370590464, 
    296638317, 229723740, 170452721, 119362028, 76914346, 43494088, 19403913, 4861986, 
};

void aec_priv_create_output(
        bfp_s32_t *output,
        bfp_s32_t *overlap,
        bfp_s32_t *error,
        const aec_config_params_t *conf)
{
    bfp_s32_t win, win_flpd;
    bfp_s32_init(&win, (int32_t*)&WOLA_window[0], -31, 32, 0);
    bfp_s32_init(&win_flpd, (int32_t*)&WOLA_window_flpd[0], -31, 32, 0);

    //zero first 240 samples
    memset(error->data, 0, AEC_FRAME_ADVANCE*sizeof(int32_t));

    bfp_s32_t chunks[2];
    bfp_s32_init(&chunks[0], &error->data[240], error->exp, 32, 0); //240-272 fwd win
    chunks[0].hr = error->hr;
    bfp_s32_init(&chunks[1], &error->data[480], error->exp, 32, 0); //480-512 flpd win
    chunks[1].hr = error->hr;

    //window error
    bfp_s32_mul(&chunks[0], &chunks[0], &win);
    bfp_s32_mul(&chunks[1], &chunks[1], &win_flpd);
    //Bring the windowed portions back to the format to the non-windowed region.
    //Here, we're assuming that the window samples are less than 1 so that windowed region can be safely brought to format of non-windowed portion without risking saturation
    bfp_s32_use_exponent(&chunks[0], error->exp);
    bfp_s32_use_exponent(&chunks[1], error->exp);
    int min_hr = (chunks[0].hr < chunks[1].hr) ? chunks[0].hr : chunks[1].hr;
    min_hr = (min_hr < error->hr) ? min_hr : error->hr;
    error->hr = min_hr;
    
    //copy error to output
    memcpy(output->data, &error->data[AEC_FRAME_ADVANCE], AEC_FRAME_ADVANCE*sizeof(int32_t));
    output->length = AEC_FRAME_ADVANCE;
    output->exp = error->exp;
    output->hr = error->hr;

    //overlap add
    //split output into 2 chunks. chunk[0] with first 32 samples of output. chunk[1] has rest of the 240-32 samples of output
    bfp_s32_init(&chunks[0], &output->data[0], output->exp, 32, 0);
    chunks[0].hr = output->hr;
    bfp_s32_init(&chunks[1], &output->data[32], output->exp, 240-32, 0);
    chunks[1].hr = output->hr;
    
    //Add previous frame's overlap to first 32 samples of output
    bfp_s32_add(&chunks[0], &chunks[0], overlap);
    bfp_s32_use_exponent(&chunks[0], -31); //bring the overlapped-added part back to 1.31
    bfp_s32_use_exponent(&chunks[1], -31); //bring the rest of output to 1.31
    output->exp = (chunks[0].exp < chunks[1].exp) ? chunks[0].exp : chunks[1].exp;
    
    //update overlap
    memcpy(overlap->data, &error->data[480], 32*sizeof(int32_t));
    overlap->hr = error->hr;
    overlap->exp = error->exp;
    return;
}

void aec_priv_calc_inverse(
        bfp_s32_t *input)
{
#if 1 //82204 cycles. 2 x-channels, single thread, but get rids of voice_toolbox dependency
    bfp_s32_inverse(input, input);
#else //36323 cycles. 2 x-channels, single thread
    int32_t min_element = xs3_vect_s32_min(
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

void aec_priv_calc_inv_X_energy_denom(
        bfp_s32_t *inv_X_energy_denom,
        const bfp_s32_t *X_energy,
        const bfp_s32_t *sigma_XX,
        const aec_config_params_t *conf,
        float_s32_t delta,
        unsigned is_shadow) {
    
    int gamma_log2 = conf->aec_core_conf.gamma_log2;
    if(!is_shadow) { //frequency smoothing
        int32_t norm_denom_buf[AEC_PROC_FRAME_LENGTH/2 + 1];
        bfp_s32_t norm_denom;
        bfp_s32_init(&norm_denom, &norm_denom_buf[0], 0, AEC_PROC_FRAME_LENGTH/2+1, 0);

        bfp_s32_t sigma_times_gamma;
        bfp_s32_init(&sigma_times_gamma, sigma_XX->data, sigma_XX->exp+gamma_log2, sigma_XX->length, 0);
        sigma_times_gamma.hr = sigma_XX->hr;
        bfp_s32_add(&norm_denom, &sigma_times_gamma, X_energy);

        //self.taps = [0.5, 1, 1, 1, 0.5] 
        fixed_s32_t taps_q30[5] = {0x20000000, 0x40000000, 0x40000000, 0x40000000, 0x20000000};
        for(int i=0; i<5; i++) {
            taps_q30[i] = taps_q30[i] >> 2;//This is equivalent to a divide by 4
        }

        bfp_s32_convolve_same(inv_X_energy_denom, &norm_denom, &taps_q30[0], 5, PAD_MODE_REFLECT);

        bfp_s32_add_scalar(inv_X_energy_denom, inv_X_energy_denom, delta);
    }
    else
    {
        bfp_s32_add_scalar(inv_X_energy_denom, X_energy, delta);
    }
}

void aec_priv_calc_inv_X_energy(
        bfp_s32_t *inv_X_energy,
        const bfp_s32_t *X_energy,
        const bfp_s32_t *sigma_XX,
        const aec_config_params_t *conf,
        float_s32_t delta,
        unsigned is_shadow)
{
    //Calculate denom for the inv_X_energy = 1/denom calculation. denom calculation is different for shadow and main filter
    aec_priv_calc_inv_X_energy_denom(inv_X_energy, X_energy, sigma_XX, conf, delta, is_shadow);
    aec_priv_calc_inverse(inv_X_energy);

    return;
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
#define Q1_30(f) ((int32_t)((double)(INT_MAX>>1) * f)) //TODO use lib_xs3_math use_exponent instead
void aec_priv_init_config_params(
        aec_config_params_t *config_params)
{
    //TODO profile double_to_float_s32() calls
    //aec_core_config_params_t
    aec_core_config_params_t *core_conf = &config_params->aec_core_conf;
    core_conf->sigma_xx_shift = 11;
    core_conf->ema_alpha_q30 = Q1_30(0.98);
    core_conf->gamma_log2 = 6;
    core_conf->delta_adaption_force_on.mant = (unsigned)UINT_MAX >> 1;
    core_conf->delta_adaption_force_on.exp = -32 - 6 + 1; //extra +1 to account for shr of 1 to the mant in order to store it as a signed number
    core_conf->delta_min = double_to_float_s32((double)1e-20);
    core_conf->bypass = 0;
    core_conf->coeff_index = 0;

    //shadow_filt_config_params_t
    shadow_filt_config_params_t *shadow_cfg = &config_params->shadow_filt_conf;
    shadow_cfg->shadow_sigma_thresh = double_to_float_s32(0.6); //# threshold for resetting sigma_xx
    shadow_cfg->shadow_copy_thresh = double_to_float_s32(0.5); //# threshold for copying shadow filter
    shadow_cfg->shadow_reset_thresh = double_to_float_s32(1.5);
    shadow_cfg->shadow_delay_thresh = double_to_float_s32(0.5); //# will not reset if reference delay is large
    shadow_cfg->x_energy_thresh = double_to_float_s32(pow(10, -40/10.0));
    shadow_cfg->shadow_better_thresh = 5; //# how many times better before copying
    shadow_cfg->shadow_zero_thresh = 5;//# zero shadow filter every n resets
    shadow_cfg->shadow_reset_timer = 20; //# number of frames between zeroing resets
    shadow_cfg->shadow_mu = double_to_float_s32(1.0);

    //coherence_mu_config_params_t 
    coherence_mu_config_params_t *coh_cfg = &config_params->coh_mu_conf;
    coh_cfg->coh_alpha = double_to_float_s32(0.0);
    coh_cfg->coh_slow_alpha = double_to_float_s32(0.99);
    coh_cfg->coh_thresh_slow = double_to_float_s32(0.9);
    coh_cfg->coh_thresh_abs = double_to_float_s32(0.65);
    coh_cfg->mu_scalar = double_to_float_s32(1.0);
    coh_cfg->eps = double_to_float_s32((double)1e-100);
    coh_cfg->thresh_minus20dB = double_to_float_s32(pow(10, -20/10.0));
    coh_cfg->x_energy_thresh = double_to_float_s32(pow(10, -40/10.0));
    coh_cfg->mu_coh_time = 2;
    coh_cfg->mu_shad_time = 5;

    coh_cfg->adaption_config = AEC_ADAPTION_AUTO;
    coh_cfg->force_adaption_mu_q30 = Q1_30(1.0);
}

void aec_priv_calc_delta(
        float_s32_t *delta, 
        const float_s32_t *max_X_energy,
        aec_config_params_t *conf,
        float_s32_t scale,
        int channels) {
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
