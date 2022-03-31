// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

typedef struct {
    double coh_alpha;
    double coh_slow_alpha;
    double coh_thresh_slow;
    double coh_thresh_abs;
    double mu_scalar;
    double eps;
    double thresh_minus20dB;
    double x_energy_thresh;

    unsigned mu_coh_time;
    unsigned mu_shad_time;
    aec_adaption_e adaption_config;
    double force_adaption_mu;

    double coh[AEC_MAX_Y_CHANNELS];
    double coh_slow[AEC_MAX_Y_CHANNELS];
    unsigned mu_coh_count[AEC_MAX_Y_CHANNELS];
    unsigned mu_shad_count[AEC_MAX_Y_CHANNELS];
} coherence_mu_params_fp;

static void init_coherence_mu_config_fp(coherence_mu_params_fp *cfg, int channels) {
    //config
    cfg->coh_alpha = 0.0;
    cfg->coh_slow_alpha = 0.99;
    cfg->coh_thresh_slow = 0.9;
    cfg->coh_thresh_abs = 0.65;
    cfg->mu_scalar = 1;
    cfg->eps = 1e-100;
    
    cfg->x_energy_thresh = -40;
    cfg->mu_coh_time = 2;
    cfg->mu_shad_time = 30;
    cfg->adaption_config = AEC_ADAPTION_AUTO;
    cfg->force_adaption_mu = 1.0;
    //state
    for(int i=0; i<channels; i++) {
        cfg->coh[i] = 1.0;
        cfg->coh_slow[i] = 0.0;
        cfg->mu_coh_count[i] = 0;
        cfg->mu_shad_count[i] = 0;
    }
}

void aec_calc_coherence_fp(
        coherence_mu_params_fp *cfg,
        double (*y)[AEC_PROC_FRAME_LENGTH],
        double (*y_hat)[AEC_PROC_FRAME_LENGTH],
        int channels,
        int bypass) {
    if(bypass) {
        return;
    }
    for(int ch=0; ch<channels; ch++) {
        double sigma_yy = 0.0;
        double sigma_yhatyhat = 0.0;
        double sigma_yyhat = 0.0;
        for(int i=240; i<480; i++) {
            sigma_yy += (y[ch][i] * y[ch][i]);
            sigma_yyhat += (y[ch][i] * y_hat[ch][i]);
            sigma_yhatyhat += (y_hat[ch][i] * y_hat[ch][i]);
        }
        //eps = 1e-100
        //this_coh = np.abs(sigma_yyhat/(np.sqrt(sigma_yy)*np.sqrt(sigma_yhatyhat) + eps))
        double denom = ((sqrt(sigma_yy) * sqrt(sigma_yhatyhat)) + cfg->eps);
        double this_coh = sigma_yyhat / denom;
        if(this_coh < 0.0) this_coh = -this_coh;

        //# moving average coherence
        //self.coh = self.coh_alpha*self.coh + (1.0 - self.coh_alpha)*this_coh
        cfg->coh[ch] = (cfg->coh_alpha * cfg->coh[ch]) + ((1.0 - cfg->coh_alpha) * this_coh);
        
        //# update slow moving averages used for thresholding
        //self.coh_slow = self.coh_slow_alpha*self.coh_slow + (1.0 - self.coh_slow_alpha)*self.coh
        cfg->coh_slow[ch] = (cfg->coh_slow_alpha * cfg->coh_slow[ch]) + ((1.0 - cfg->coh_slow_alpha) * cfg->coh[ch]);
    }
}

void test_calc_coherence() {
    unsigned num_y_channels = 1;
    unsigned num_x_channels = 1;
    unsigned num_phases = AEC_MAIN_FILTER_PHASES - 1;

    aec_memory_pool_t aec_memory_pool;
    aec_state_t state;
    aec_shared_state_t aec_shared_state;
    aec_init(&state, NULL, &aec_shared_state, (uint8_t*)&aec_memory_pool, NULL, num_y_channels, num_x_channels, num_phases, 0);
    
    //Initialize floating point
    coherence_mu_params_fp coh_params_fp;
    init_coherence_mu_config_fp(&coh_params_fp, num_y_channels);
    double y_fp[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH], y_hat_fp[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH];
    
    int32_t new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
    unsigned seed = 10;
    int32_t max_diff = 0; 
    for(int iter=0; iter<(1<<12)/F; iter++) {
        state.shared_state->config_params.aec_core_conf.bypass = pseudo_rand_uint32(&seed) % 2;
        aec_frame_init(&state, NULL, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]); //frame init will copy y[240:480] into output
        //state.y_hat is initialised as part of Y_hat -> y_hat ifft. Do it here for standalone testing
        for(int ch=0; ch<num_y_channels; ch++) {
            bfp_s32_init(&state.y_hat[ch], (int32_t*)&state.Y_hat[ch].data[0], 0, AEC_PROC_FRAME_LENGTH, 0);
        }

        for(int ch=0; ch<num_y_channels; ch++) {
            state.shared_state->y[ch].exp = pseudo_rand_int(&seed, -31, 32);
            state.shared_state->y[ch].hr = pseudo_rand_uint32(&seed) % 4;

            state.y_hat[ch].exp = pseudo_rand_int(&seed, -31, 32);
            state.y_hat[ch].hr = pseudo_rand_uint32(&seed) % 4;

            for(int i=0; i<AEC_PROC_FRAME_LENGTH; i++) {
                state.shared_state->y[ch].data[i] = pseudo_rand_int32(&seed) >> state.shared_state->y[ch].hr;
                y_fp[ch][i] = ldexp(state.shared_state->y[ch].data[i], state.shared_state->y[ch].exp);

                state.y_hat[ch].data[i] = pseudo_rand_int32(&seed) >> state.y_hat[ch].hr;
                y_hat_fp[ch][i] = ldexp(state.y_hat[ch].data[i], state.y_hat[ch].exp);
            }
        }

        //since state.shared_state->y is being initialised with a new frame after calling aec_frame_init(), we need to update state->shared_state->prev_y again since that's where y[240:480] is read from in aec_calc_coherence()
        for(int ch=0; ch<num_y_channels; ch++) {
            memcpy(state.shared_state->prev_y[ch].data, &state.shared_state->y[ch].data[AEC_FRAME_ADVANCE], (AEC_PROC_FRAME_LENGTH-AEC_FRAME_ADVANCE)*sizeof(int32_t));
            state.shared_state->prev_y[ch].exp = state.shared_state->y[ch].exp;
            state.shared_state->prev_y[ch].hr = state.shared_state->y[ch].hr;
        }


        aec_calc_coherence_fp(&coh_params_fp, y_fp, y_hat_fp, num_y_channels, state.shared_state->config_params.aec_core_conf.bypass);

        for(int ch=0; ch<num_y_channels; ch++) {
            aec_calc_coherence(&state, ch);
            
        }
        for(int ch=0; ch<num_y_channels; ch++) {
            coherence_mu_params_t *coh_mu_state_ptr = &state.shared_state->coh_mu_state[ch];

            //printf("coh: %f, %f, (%d, %d)\n", coh_params_fp.coh[ch], ldexp(coh_mu_state_ptr->coh.mant, coh_mu_state_ptr->coh.exp), coh_mu_state_ptr->coh.mant, coh_mu_state_ptr->coh.exp);
            int32_t dut_coh = coh_mu_state_ptr->coh.mant;
            int32_t ref_coh = double_to_int32(coh_params_fp.coh[ch], coh_mu_state_ptr->coh.exp);
            int diff = abs(ref_coh - dut_coh);
            TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<13, diff, "coh diff too large.");
            if(diff > max_diff) max_diff = diff;

            //printf("coh_slow: %f, %f, (%d, %d)\n", coh_params_fp.coh_slow[ch], ldexp(coh_mu_state_ptr->coh_slow.mant, coh_mu_state_ptr->coh_slow.exp), coh_mu_state_ptr->coh_slow.mant, coh_mu_state_ptr->coh_slow.exp);
            int32_t dut_coh_slow = coh_mu_state_ptr->coh_slow.mant;
            int32_t ref_coh_slow = double_to_int32(coh_params_fp.coh_slow[ch], coh_mu_state_ptr->coh_slow.exp);
            diff = abs(ref_coh_slow - dut_coh_slow);
            if(diff > max_diff) max_diff = diff;
            TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<13, diff, "coh slow diff too large.");
        }
        
    }
    printf("max_diff = %ld\n",max_diff);
}
