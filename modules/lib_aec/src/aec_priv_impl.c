// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_priv.h"
#include "q_format.h"

#include "fdaf_api.h"

void aec_priv_main_init(
        aec_state_t *state,
        aec_shared_state_t *shared_state,
        uint8_t *mem_pool,
        unsigned num_y_channels,
        unsigned num_x_channels,
        unsigned num_phases)
{ 
    memset(state, 0, sizeof(aec_state_t));
    // Reset shared_state. Only done in main_init()
    memset(shared_state, 0, sizeof(aec_shared_state_t));

    uint8_t *available_mem_start = (uint8_t*)mem_pool;

    state->shared_state = shared_state;
    // Initialise number of y and x channels
    state->shared_state->num_y_channels = num_y_channels;
    state->shared_state->num_x_channels = num_x_channels;
    // Initialise number of phases
    state->num_phases = num_phases;

    // y
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->shared_state->y[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH), 0); //input data is 1.31 so initialising with exp AEC_INPUT_EXP
        available_mem_start += ((AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING)*sizeof(int32_t)); //2 extra samples of memory allocated. state->shared_state->y[ch].length is still AEC_PROC_FRAME_LENGTH though
    }
    // x
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->shared_state->x[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH), 0); //input data is 1.31 so initialising with exp -31
        available_mem_start += ((AEC_PROC_FRAME_LENGTH + AEC_FFT_PADDING)*sizeof(int32_t)); //2 extra samples of memory allocated. state->shared_state->x[ch].length is still AEC_PROC_FRAME_LENGTH though
    }
    // prev_y
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->shared_state->prev_y[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE), 0); //input data is 1.31 so initialising with exp -31
        available_mem_start += ((AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE)*sizeof(int32_t));
    }
    // prev_x
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->shared_state->prev_x[ch], (int32_t*)available_mem_start, AEC_INPUT_EXP, (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE), 0); //input data is 1.31 so initialising with exp -31
        available_mem_start += ((AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE)*sizeof(int32_t));
    }
    
    // H_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        for(unsigned ph=0; ph<(num_x_channels * num_phases); ph++) {
            bfp_complex_s32_init(&state->H_hat[ch][ph], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
            available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
        }
    }
    // X_fifo
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        for(unsigned ph=0; ph<num_phases; ph++) {
            bfp_complex_s32_init(&state->shared_state->X_fifo[ch][ph], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
            available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
        }
    }
    // Initialise Error
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Error[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }
    // Initiaise Y_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Y_hat[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }

    // X_energy 
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0); 
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }
    // sigma_XX
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->shared_state->sigma_XX[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }
    // inv_X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->inv_X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }

    // overlap
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->overlap[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, 32, 0);
        available_mem_start += (32*sizeof(int32_t)); 
    }
    uint32_t memory_used = available_mem_start - (uint8_t*)mem_pool; 
    memset(mem_pool, 0, memory_used);

    // Initialise ema energy
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        state->shared_state->y_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
        state->error_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
    }
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        state->shared_state->x_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
    }
    // Fractional regularisation scale factor
    state->delta_scale = double_to_float_s32((double)1e-5);

    // Initialise aec config params
    aec_priv_init_config_params(&state->shared_state->config_params);

    // Initialise coherence mu params
    coherence_mu_params_t *coh_params = state->shared_state->coh_mu_state;
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        coh_params[ch].coh = double_to_float_s32(1.0);
        coh_params[ch].coh_slow = double_to_float_s32(0.0);
        coh_params[ch].mu_coh_count = 0;
        coh_params[ch].mu_shad_count = 0;
    }

    // Initialise shadow filter params
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
    
    // Initialise number of phases
    state->num_phases = num_phases;

    state->shared_state = shared_state;
    unsigned num_y_channels = state->shared_state->num_y_channels;
    unsigned num_x_channels = state->shared_state->num_x_channels;

    // H_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        for(unsigned ph=0; ph<(num_x_channels * num_phases); ph++) {
            bfp_complex_s32_init(&state->H_hat[ch][ph], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
            available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
        }
    }
    // Initialise Error
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Error[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }
    // Initiaise Y_hat
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_complex_s32_init(&state->Y_hat[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }
    // Initialise T
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_complex_s32_init(&state->T[ch], (complex_s32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(complex_s32_t)); 
    }

    // X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
       bfp_s32_init(&state->X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0); 
       available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }
    // inv_X_energy
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&state->inv_X_energy[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, AEC_FD_FRAME_LENGTH, 0);
        available_mem_start += (AEC_FD_FRAME_LENGTH*sizeof(int32_t)); 
    }

    // overlap
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&state->overlap[ch], (int32_t*)available_mem_start, AEC_ZEROVAL_EXP, 32, 0);
        available_mem_start += (32*sizeof(int32_t)); 
    }

    uint32_t memory_used = available_mem_start - (uint8_t*)mem_pool; 
    memset(mem_pool, 0, memory_used);

    // Initialise ema energy
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        state->error_ema_energy[ch].exp = AEC_ZEROVAL_EXP;
    }
    // Fractional regularisation scale factor
    state->delta_scale = double_to_float_s32((double)1e-3);
}

void aec_priv_bfp_complex_s32_copy(
        bfp_complex_s32_t *dst,
        const bfp_complex_s32_t *src)
{
    // This assumes that both dst and src are same length
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
    // Copy the H_hat_src phases into H_hat_dst
    for(uint32_t ch=0; ch<num_x_channels; ch++) {
        uint32_t dst_ph_start_offset = ch * num_dst_phases;
        uint32_t src_ph_start_offset = ch * num_src_phases;
        for(uint32_t ph=0; ph<phases_to_copy; ph++) {
            aec_priv_bfp_complex_s32_copy(&H_hat_dst[dst_ph_start_offset + ph], &H_hat_src[src_ph_start_offset + ph]);
        }
    }
    // Zero the remaining H_hat_dst phases
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
        // Check if shadow or reference filter will be used and flag accordingly
        if(ref_low_all_xch) {
            // Input level is low, so error is unreliable, do nothing
            shadow_params->shadow_flag[ch] = LOW_REF;
            continue;
        }
        // If error way bigger than input, reset- should percolate through to main filter if better
        if(float_s32_gt(shadow_state->overall_Error[ch], shared_state->overall_Y[ch]) && shadow_params->shadow_reset_count[ch] >= 0)
        {
            shadow_params->shadow_flag[ch] = ERROR;
            fdaf_reset_filter(shadow_state->H_hat[ch], shadow_state->shared_state->num_x_channels, shadow_state->num_phases);
            // Y -> shadow Error
            aec_priv_bfp_complex_s32_copy(&shadow_state->Error[ch], &shared_state->Y[ch]);
            shadow_state->overall_Error[ch] = shared_state->overall_Y[ch];
            // Give the zeroed filter time to reconverge (or redeconverge)
            shadow_params->shadow_reset_count[ch] = -(int)shadow_conf->shadow_reset_timer;
        }
        if(float_s32_gte(shadow_copy_thresh_x_Ov_Error, shadow_state->overall_Error[ch]) &&
                (shadow_params->shadow_better_count[ch] > shadow_conf->shadow_better_thresh)) {
            // If shadow filter is much better, and has been for several frames,
            // Copy to reference filter
            shadow_params->shadow_flag[ch] = COPY;
            shadow_params->shadow_reset_count[ch] = 0;
            shadow_params->shadow_better_count[ch] += 1;
            // Shadow Error -> Error
            aec_priv_bfp_complex_s32_copy(&main_state->Error[ch], &shadow_state->Error[ch]);
            // Shadow filter -> main filter
            aec_priv_copy_filter(main_state->H_hat[ch], shadow_state->H_hat[ch], main_state->shared_state->num_x_channels, main_state->num_phases, shadow_state->num_phases);
        }
        else if(float_s32_gte(shadow_sigma_thresh_x_Ov_Error, shadow_state->overall_Error[ch]))
        {
            shadow_params->shadow_better_count[ch] += 1;
            if(shadow_params->shadow_better_count[ch] > shadow_conf->shadow_better_thresh) {
                // If shadow is somewhat better, reset sigma_xx if both channels are better
                shadow_params->shadow_flag[ch] = SIGMA;
            }
            else {
                shadow_params->shadow_flag[ch] = EQUAL;
            }
        }
        else if(float_s32_gte(shadow_state->overall_Error[ch], shadow_reset_thresh_x_Ov_Error) && 
                shadow_params->shadow_reset_count[ch] >= 0)
        {
            // If shadow filter is worse than reference, reset provided that
            // The delay is small and we're not letting the shadow filter reconverge after zeroing
            shadow_params->shadow_reset_count[ch] += 1;
            shadow_params->shadow_better_count[ch] = 0;
            if(shadow_params->shadow_reset_count[ch] > shadow_conf->shadow_zero_thresh) {
                // If shadow filter has been reset several times in a row, reset to zeros
                shadow_params->shadow_flag[ch] = ZERO;
                fdaf_reset_filter(shadow_state->H_hat[ch], shadow_state->shared_state->num_x_channels, shadow_state->num_phases);
                aec_priv_bfp_complex_s32_copy(&shadow_state->Error[ch], &shared_state->Y[ch]);
                // Give the zeroed filter time to reconverge (or redeconverge)
                shadow_params->shadow_reset_count[ch] = -(int)shadow_conf->shadow_reset_timer;
            }
            else {
                //debug_printf("Frame %d, main -> shadow filter copy.\n",frame_counter);
                // Otherwise copy the main filter to the shadow filter
                aec_priv_copy_filter(shadow_state->H_hat[ch], main_state->H_hat[ch], main_state->shared_state->num_x_channels, shadow_state->num_phases, main_state->num_phases);
                aec_priv_bfp_complex_s32_copy(&shadow_state->Error[ch], &main_state->Error[ch]);
                shadow_params->shadow_flag[ch] = RESET;
            }
        }
        else {
            // Shadow filter is comparable to main filter, 
            // or we're waiting for it to reconverge after zeroing
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
    // If the coherence has been low within the last 15 frames, keep the count != 0
    for(unsigned ch=0; ch<num_y_channels; ch++)
    {
        if(coh_mu_state[ch].mu_coh_count > 0) {
            coh_mu_state[ch].mu_coh_count += 1;
        }
        if(coh_mu_state[ch].mu_coh_count > coh_conf->mu_coh_time) {
            coh_mu_state[ch].mu_coh_count = 0;
        }
    }
    // If the shadow filter has be en used within the last 15 frames, keep the count != 0
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
    // Threshold for coherence between y and y_hat
    float_s32_t min_coh_slow = coh_mu_state[0].coh_slow;
    for(unsigned ch=1; ch<num_y_channels; ch++)
    {
        if(float_s32_gt(min_coh_slow, coh_mu_state[ch].coh_slow)) {
            min_coh_slow = coh_mu_state[ch].coh_slow;
        }
    }
    // Threshold for coherence between y and y_hat
    float_s32_t CC_thres = float_s32_mul(min_coh_slow, coh_conf->coh_thresh_slow);
    for(unsigned ch=0; ch<num_y_channels; ch++)
    {
        if(shadow_flag[ch] >= SIGMA) {
            // If the shadow filter has triggered, override any drop in coherence
            coh_mu_state[ch].mu_coh_count = 0;
        }
        else {
            // Otherwise if the coherence is low start the count
            if(float_s32_gt(coh_conf->coh_thresh_abs, coh_mu_state[ch].coh)) {
                coh_mu_state[ch].mu_coh_count = 1;
            }
        }
    }
    if(coh_conf->adaption_config == AEC_ADAPTION_AUTO){
        // Order of priority for coh_mu:
        // 1) if the reference energy is low, don't converge (not enough SNR to be accurate)
        // 2) if shadow filter has triggered recently, converge fast
        // 3) if coherence has dropped recently, don't converge
        // 4) otherwise, converge fast.
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
            else { // If yy_hat coherence denotes absence of near-end/noise
                if(float_s32_gt(coh_mu_state[ch].coh, coh_mu_state[ch].coh_slow)) {
                    for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
                        coh_mu_state[ch].coh_mu[x_ch] = double_to_float_s32(1.0);
                    }
                }
                else if(float_s32_gt(coh_mu_state[ch].coh, CC_thres))
                {
                    // scale coh_mu depending on how far above the threshold it is
                    // self.mu[y_ch] = ((self.coh[y_ch]-CC_thres)/(self.coh_slow[y_ch]-CC_thres))**2
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
        // np.max(ref_energy_log)-20 is done as (max_ref_energy_not_log*(pow(10, -20/10)))
        float_s32_t max_ref_energy_minus_20dB = float_s32_mul(max_ref_energy, coh_conf->thresh_minus20dB);
        for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
            // if ref_energy_log[x_ch] <= ref_energy_thresh or ref_energy_log[x_ch] < np.max(ref_energy_log)-20: 
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
                const int32_t q30_exp = -30;
                coh_mu_state[y_ch].coh_mu[x_ch].exp = q30_exp;
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
    /*for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {
      for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
        printf("mu[%d][%d] = %f\n",y_ch, x_ch, float_s32_to_double(coh_mu_state[y_ch].coh_mu[x_ch]));
        
      }
    }*/
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
    fdaf_l2_calc_Error_and_Y_hat(Error, Y_hat, Y, X_fifo, H_hat, num_x_channels, num_phases, 0, AEC_PROC_FRAME_LENGTH/2 + 1, bypass_enabled);
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

    // Calculate coherence between y and y_hat
    // eps = 1e-100
    // this_coh = np.abs(sigma_yyhat/(np.sqrt(sigma_yy)*np.sqrt(sigma_yhatyhat) + eps))
    float_s32_t denom;
    denom = float_s32_mul(sigma_yy, sigma_yhatyhat);
    denom = float_s32_sqrt(denom);
    denom = float_s32_add(denom, coh_conf->eps);
    if(denom.mant == 0) {
        denom = coh_conf->eps;
    }

    float_s32_t this_coh = float_s32_div(float_s32_abs(sigma_yyhat), denom);

    // moving average coherence
    // self.coh = self.coh_alpha*self.coh + (1.0 - self.coh_alpha)*this_coh
    float_s32_t one = double_to_float_s32(1.0); //TODO profile this call
    float_s32_t one_minus_alpha = float_s32_sub(one, coh_conf->coh_alpha);
    float_s32_t t1 = float_s32_mul(coh_conf->coh_alpha, coh_mu_state->coh);
    float_s32_t t2 = float_s32_mul(one_minus_alpha, this_coh);
    coh_mu_state->coh = float_s32_add(t1, t2);

    // update slow moving averages used for thresholding
    // self.coh_slow = self.coh_slow_alpha*self.coh_slow + (1.0 - self.coh_slow_alpha)*self.coh
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

void aec_priv_filter_adapt(
        bfp_complex_s32_t *H_hat,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *T,
        unsigned num_x_channels,
        unsigned num_phases)
{
    unsigned phases = num_x_channels * num_phases;
    for(unsigned ph=0; ph<phases; ph++) {
        // Find out which channel this phase belongs to
        fdaf_l2_adapt_plus_fft_gc(&H_hat[ph], &X_fifo[ph], &T[ph/num_phases]);
    }
}

void aec_priv_init_config_params(
        aec_config_params_t *config_params)
{
    // TODO profile double_to_float_s32() calls
    // aec_core_config_params_t
    aec_core_config_params_t *core_conf = &config_params->aec_core_conf;
    core_conf->sigma_xx_shift = 6;
    core_conf->ema_alpha_q30 = Q30(0.98);
    core_conf->gamma_log2 = 5;
    core_conf->delta_adaption_force_on.mant = (unsigned)UINT_MAX >> 1;
    core_conf->delta_adaption_force_on.exp = -32 - 6 + 1; //extra +1 to account for shr of 1 to the mant in order to store it as a signed number
    core_conf->delta_min = double_to_float_s32((double)1e-20);
    core_conf->bypass = 0;
    core_conf->coeff_index = 0;

    // shadow_filt_config_params_t
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

    // coherence_mu_config_params_t 
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
    coh_cfg->force_adaption_mu_q30 = Q30(0.1);
}
