// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

#define TEST_NUM_Y (2)
#define TEST_NUM_X (2)
#define TEST_MAIN_PHASES (3)
#define TEST_SHADOW_PHASES (1)
#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)

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
}coherence_mu_config_fp_t;

typedef struct {
    double shadow_sigma_thresh;
    double shadow_copy_thresh;
    double shadow_reset_thresh;
    double shadow_delay_thresh;
    double x_energy_thresh;
    double shadow_mu;

    int shadow_better_thresh;
    int shadow_zero_thresh;
    int shadow_reset_timer;
}shadow_filt_config_fp_t;


typedef struct {
    //shadow filter
    int shadow_flag[AEC_MAX_Y_CHANNELS];
    int shadow_reset_count[AEC_MAX_Y_CHANNELS];
    int shadow_better_count[AEC_MAX_Y_CHANNELS];

    //coherence mu
    double coh[AEC_MAX_Y_CHANNELS];
    double coh_slow[AEC_MAX_Y_CHANNELS];
    int mu_coh_count[AEC_MAX_Y_CHANNELS];
    int mu_shad_count[AEC_MAX_Y_CHANNELS];
    double coh_mu[AEC_MAX_Y_CHANNELS][AEC_MAX_X_CHANNELS];

    //common
    complex_double_t H_hat[TEST_NUM_Y][TEST_NUM_X][TEST_MAIN_PHASES][NUM_BINS];
    complex_double_t H_hat_shadow[TEST_NUM_Y][TEST_NUM_X][TEST_SHADOW_PHASES][NUM_BINS];
    complex_double_t Y[TEST_NUM_Y][NUM_BINS];
    complex_double_t Error[TEST_NUM_Y][NUM_BINS];
    complex_double_t Error_shadow[TEST_NUM_Y][NUM_BINS];
    double sigma_XX[TEST_NUM_X][NUM_BINS];
    double main_filt_mu[AEC_MAX_Y_CHANNELS][AEC_MAX_X_CHANNELS];
    double shadow_filt_mu[AEC_MAX_Y_CHANNELS][AEC_MAX_X_CHANNELS];
    double overall_Error[AEC_MAX_Y_CHANNELS];
    double overall_Error_shadow[AEC_MAX_Y_CHANNELS];
    double overall_Y[AEC_MAX_Y_CHANNELS];
    double sum_X_energy[AEC_MAX_X_CHANNELS];
    double max_X_energy_main[AEC_MAX_X_CHANNELS];
    double max_X_energy_shadow[AEC_MAX_X_CHANNELS];
    double delta_min;
    double delta_adaption_force_on;
    double delta_scale_main;
    double delta_scale_shadow;
    double delta_main;
    double delta_shadow;
    int main_phases;
    int shadow_phases;
    int y_channels;
    int x_channels;
}shadow_filt_params_fp_t;

static void init_shadow_config_fp(shadow_filt_config_fp_t *cfg) {
    cfg->shadow_sigma_thresh = 0.6;
    cfg->shadow_copy_thresh = 0.5;
    cfg->shadow_reset_thresh = 1.5;
    cfg->shadow_delay_thresh = 0.5;
    cfg->x_energy_thresh = pow(10, -40/10);
    cfg->shadow_mu = 1.0;
    cfg->shadow_better_thresh = 5;
    cfg->shadow_zero_thresh = 5;
    cfg->shadow_reset_timer = 20;
}

static void init_coherence_mu_config_fp(coherence_mu_config_fp_t *cfg) {
    //config
    cfg->coh_alpha = 0.0;
    cfg->coh_slow_alpha = 0.99;
    cfg->coh_thresh_slow = 0.9;
    cfg->coh_thresh_abs = 0.65;
    cfg->mu_scalar = 1.0;
    cfg->eps = (double)1e-100;
    
    cfg->thresh_minus20dB = pow(10, -20/10);
    cfg->x_energy_thresh = pow(10, -40/10);
    cfg->mu_coh_time = 2;
    cfg->mu_shad_time = 5;
    cfg->adaption_config = AEC_ADAPTION_AUTO;
    cfg->force_adaption_mu = 1.0;
}

static void init_params_fp(
        shadow_filt_params_fp_t *params,
        const shadow_filt_config_fp_t *shadow_cfg,
        const coherence_mu_config_fp_t *coh_mu_cfg,
        unsigned num_y_channels,
        unsigned num_x_channels,
        unsigned main_filter_phases,
        unsigned shadow_filter_phases)
{
    params->main_phases = main_filter_phases;
    params->shadow_phases = shadow_filter_phases;
    params->y_channels = num_y_channels;
    params->x_channels = num_x_channels;
    params->delta_min = (double)1e-20;
    params->delta_adaption_force_on = ldexp(UINT_MAX, -32-6);
    params->delta_scale_main = (double)1e-5;
    params->delta_scale_shadow = (double)1e-3;

    for(int ch=0; ch<params->y_channels; ch++) {
        params->shadow_flag[ch] = EQUAL;
        params->shadow_reset_count[ch] = -shadow_cfg->shadow_reset_timer;
        params->shadow_better_count[ch] = 0;

        params->coh[ch] = 1.0;
        params->coh_slow[ch] = 0.0;
        params->mu_coh_count[ch] = 0;
        params->mu_shad_count[ch] = 0;
    }
}

void reset_shadow_filter_fp(shadow_filt_params_fp_t *params, int y_ch) {
    for(int ch=0; ch<params->x_channels; ch++) {
        for(int ph=0; ph<params->shadow_phases; ph++) {
            for(int bin=0; bin<NUM_BINS; bin++) {
                params->H_hat_shadow[y_ch][ch][ph][bin].re = 0.0;
                params->H_hat_shadow[y_ch][ch][ph][bin].im = 0.0;
            }
        }
    }
}

void reset_main_filter_fp(shadow_filt_params_fp_t *params, int y_ch) {
    printf("reset_main_filter_fp. ych %d\n",y_ch);
    for(int ch=0; ch<params->x_channels; ch++) {
        for(int ph=0; ph<params->main_phases; ph++) {
            for(int bin=0; bin<NUM_BINS; bin++) {
                params->H_hat[y_ch][ch][ph][bin].re = 0.0;
                params->H_hat[y_ch][ch][ph][bin].im = 0.0;
            }
        }
    }
}

void shadow_to_main_filter_copy_fp(shadow_filt_params_fp_t *params, int y_ch) {
    for(int ch=0; ch<params->x_channels; ch++) {
        for(int ph=0; ph<params->shadow_phases; ph++) {
            for(int bin=0; bin<NUM_BINS; bin++) {
                params->H_hat[y_ch][ch][ph][bin].re = params->H_hat_shadow[y_ch][ch][ph][bin].re;
                params->H_hat[y_ch][ch][ph][bin].im = params->H_hat_shadow[y_ch][ch][ph][bin].im;
            }
        }
        for(int ph=params->shadow_phases; ph<params->main_phases; ph++) {
            for(int bin=0; bin<NUM_BINS; bin++) {
                params->H_hat[y_ch][ch][ph][bin].re = 0.0;
                params->H_hat[y_ch][ch][ph][bin].im = 0.0;
            }
        }
    }
}

void main_to_shadow_filter_copy_fp(shadow_filt_params_fp_t *params, int y_ch) {
    for(int ch=0; ch<params->x_channels; ch++) {
        for(int ph=0; ph<params->shadow_phases; ph++) {
            for(int bin=0; bin<NUM_BINS; bin++) {
                params->H_hat_shadow[y_ch][ch][ph][bin].re = params->H_hat[y_ch][ch][ph][bin].re;
                params->H_hat_shadow[y_ch][ch][ph][bin].im = params->H_hat[y_ch][ch][ph][bin].im;
            }
        }
    }
}

#define NUM_MU_CHECKPOINTS (15)
int checkpoints_mu[NUM_MU_CHECKPOINTS] = {0};
void calc_coherence_mu_fp(
        shadow_filt_params_fp_t *params,
        const coherence_mu_config_fp_t *cfg)
{
    double *sum_X_energy = params->sum_X_energy;
    //# If the coherence has been low within the last 15 frames, keep the count != 0
    for(int ch=0; ch<params->y_channels; ch++) {
        if(params->mu_coh_count[ch] > 0) {
            checkpoints_mu[0] |= 1;
            params->mu_coh_count[ch] += 1;
        }
        if(params->mu_coh_count[ch] > cfg->mu_coh_time) {
            checkpoints_mu[1] |= 1;
            params->mu_coh_count[ch] = 0;
        }
    }
    //# If the shadow filter has be en used within the last 15 frames, keep the count != 0
    for(int ch=0; ch<params->y_channels; ch++) {
        if(params->shadow_flag[ch] == COPY) {
            checkpoints_mu[2] |= 1;
            params->mu_shad_count[ch] = 1;
        }
        else if(params->mu_shad_count[ch] > 0) {
            checkpoints_mu[3] |= 1;
            params->mu_shad_count[ch] += 1;
        }
        if(params->mu_shad_count[ch] > cfg->mu_shad_time) {
            checkpoints_mu[4] |= 1;
            params->mu_shad_count[ch] = 0;
        }
    }
    //# threshold for coherence between y and y_hat
    double min_coh_slow = params->coh_slow[0];
    for(int ch=1; ch<params->y_channels; ch++) {
        if(params->coh_slow[ch] < min_coh_slow) min_coh_slow = params->coh_slow[ch];
    }
    double CC_thres = min_coh_slow * cfg->coh_thresh_slow;
    for(int ch=0; ch<params->y_channels; ch++) {
        if(params->shadow_flag[ch] >= SIGMA) {
            checkpoints_mu[5] |= 1;
            //# if the shadow filter has triggered, override any drop in coherence
            params->mu_coh_count[ch] = 0;
        }
        else {
            //# otherwise if the coherence is low start the count
            if(params->coh[ch] < cfg->coh_thresh_abs) {
                checkpoints_mu[6] |= 1;
                params->mu_coh_count[ch] = 1;
            }
        }
    }
    if(cfg->adaption_config == AEC_ADAPTION_AUTO) {
        //# Order of priority for mu:
        //# 1) if the reference energy is low, don't converge (not enough SNR to be accurate)
        //# 2) if shadow filter has triggered recently, converge fast
        //# 3) if coherence has dropped recently, don't converge
        //# 4) otherwise, converge fast.
        for(int ch=0; ch<params->y_channels; ch++) {
            if(params->mu_shad_count[ch] >= 1) {
                //printf("here 1\n");
                checkpoints_mu[7] |= 1;
                for(int xch=0; xch<params->x_channels; xch++) {
                    params->coh_mu[ch][xch] = 1.0;
                }
            }
            else if(params->mu_coh_count[ch] > 0) {
                //printf("here 2\n");
                checkpoints_mu[8] |= 1;
                for(int xch=0; xch<params->x_channels; xch++) {
                    params->coh_mu[ch][xch] = 0.0;
                }
            }
            else { //# if yy_hat coherence denotes absence of near-end/noise
                //printf("here 3\n");
                //printf("coh %f, coh_slow %f, CC_thres %f\n",params->coh[ch], params->coh_slow[ch], CC_thres);
                if(params->coh[ch] > params->coh_slow[ch]) {
                    checkpoints_mu[9] |= 1;
                    for(int xch=0; xch<params->x_channels; xch++) {
                        params->coh_mu[ch][xch] = 1.0;
                    }
                }
                else if(params->coh[ch] > CC_thres) {
                    checkpoints_mu[10] |= 1;
                    //# scale mu depending on how far above the threshold it is
                    //self.mu[y_ch] = ((self.coh[y_ch]-CC_thres)/(self.coh_slow[y_ch]-CC_thres))**2
                    double t = (params->coh[ch] - CC_thres) / (params->coh_slow[ch] - CC_thres);
                    t = t * t;
                    for(int xch=0; xch<params->x_channels; xch++) {
                        params->coh_mu[ch][xch] = t;
                    }
                }
                else { //# shouldn't go through here, but if it does coherence is low so don't adapt
                    checkpoints_mu[11] |= 1;
                    for(int xch=0; xch<params->x_channels; xch++) {
                        params->coh_mu[ch][xch] = 0.0;
                    }
                }
            }
        }
        double sum_X_energy_max = sum_X_energy[0]; 
        for(int xch=1; xch<params->x_channels; xch++) {
            if(sum_X_energy_max < sum_X_energy[xch]) sum_X_energy_max = sum_X_energy[xch];
        }
        for(int xch=0; xch<params->x_channels; xch++) {
            //if ref_energy_log[x_ch] <= ref_energy_thresh or ref_energy_log[x_ch] < np.max(ref_energy_log)-20: 
            if((sum_X_energy[xch] <= cfg->x_energy_thresh) || (sum_X_energy[xch] < (sum_X_energy_max * cfg->thresh_minus20dB))) {
                checkpoints_mu[12] |= 1;
                for(int ych=0; ych<params->y_channels; ych++) {
                    params->coh_mu[ych][xch] = 0.0;
                }
            }
        }
        for(int ych=0; ych<params->y_channels; ych++) {
            for(int xch=0; xch<params->x_channels; xch++) {
                params->coh_mu[ych][xch] = params->coh_mu[ych][xch] * cfg->mu_scalar;
            }
        }
    }
    if(cfg->adaption_config == AEC_ADAPTION_FORCE_ON) {
        checkpoints_mu[13] |= 1;
        for(int ych=0; ych<params->y_channels; ych++) {
            for(int xch=0; xch<params->x_channels; xch++) {
                params->coh_mu[ych][xch] = cfg->force_adaption_mu;
            }
        }
    }
    else if(cfg->adaption_config == AEC_ADAPTION_FORCE_OFF) {
        checkpoints_mu[14] |= 1;
        for(int ych=0; ych<params->y_channels; ych++) {
            for(int xch=0; xch<params->x_channels; xch++) {
                params->coh_mu[ych][xch] = 0.0;
            }
        }
    }
}

#define NUM_SHADOW_CHECKPOINTS (9)
int checkpoints[NUM_SHADOW_CHECKPOINTS] = {0};
void compare_filter_fp(
        shadow_filt_params_fp_t *params,        
        const shadow_filt_config_fp_t *cfg
        )
{
    double *overall_Error = params->overall_Error;
    double *overall_Error_shadow = params->overall_Error_shadow;
    double *overall_Input = params->overall_Y;
    double *sum_X_energy = params->sum_X_energy;

    //# check if shadow or reference filter will be used and flag accordingly
    int ref_low_all = 1;
    for(int i=0; i<params->x_channels; i++) {
        if(sum_X_energy[i] >= cfg->x_energy_thresh) {
            ref_low_all = 0;
            break;
        }
    }
    for(int ch=0; ch<params->y_channels; ch++) {
        overall_Input[ch] = overall_Input[ch] / 2;
        if(ref_low_all) {
            //printf("checkpoint 0\n");
            checkpoints[0] |= 1;
            params->shadow_flag[ch] = LOW_REF;
            continue;
        }
        //# if error way bigger than input, reset- should percolate through to main filter if better
        if((overall_Error_shadow[ch] > overall_Input[ch]) && (params->shadow_reset_count[ch] >= 0)) {
            //printf("REF checkpoint 1. ych %d\n", ch);
            checkpoints[1] |= 1;
            params->shadow_flag[ch] = ERROR;
            reset_shadow_filter_fp(params, ch);
            for(int i=0; i<NUM_BINS; i++) {
                params->Error_shadow[ch][i].re = params->Y[ch][i].re;
                params->Error_shadow[ch][i].im = params->Y[ch][i].im;
            }
            overall_Error_shadow[ch] = overall_Input[ch]; 
            //# give the zeroed filter time to reconverge (or redeconverge)
            params->shadow_reset_count[ch] = -cfg->shadow_reset_timer;             
        }
        if((overall_Error_shadow[ch] <= (cfg->shadow_copy_thresh * overall_Error[ch])) &&
            (params->shadow_better_count[ch] > cfg->shadow_better_thresh)) {
            checkpoints[2] |= 1;
            //printf("checkpoint 2\n");
            //# if shadow filter is much better, and has been for several frames,
            //# copy to reference filter                            
            params->shadow_flag[ch] = COPY;
            params->shadow_reset_count[ch] = 0;
            params->shadow_better_count[ch] += 1;
            for(int i=0; i<NUM_BINS; i++) {
                params->Error[ch][i].re = params->Error_shadow[ch][i].re;
                params->Error[ch][i].im = params->Error_shadow[ch][i].im;
            }
            shadow_to_main_filter_copy_fp(params, ch);
        }
        else if(overall_Error_shadow[ch] <= cfg->shadow_sigma_thresh*overall_Error[ch]) {
            params->shadow_better_count[ch] += 1;
            if(params->shadow_better_count[ch] > cfg->shadow_better_thresh) {
                //# if shadow is somewhat better, reset sigma_xx if both channels are better
                //printf("checkpoint 3\n");
                checkpoints[3] |= 1;
                params->shadow_flag[ch] = SIGMA;
            }
            else {
                //printf("checkpoint 4\n");
                checkpoints[4] |= 1;
                params->shadow_flag[ch] = EQUAL;
            }
        }
        else if((overall_Error_shadow[ch] >= cfg->shadow_reset_thresh * overall_Error[ch]) && 
            (params->shadow_reset_count[ch] >= 0)) {
            //# if shadow filter is worse than reference, reset provided that
            //# the delay is small and we're not letting the shadow filter reconverge after zeroing
            params->shadow_reset_count[ch] += 1;
            params->shadow_better_count[ch] = 0;
            if(params->shadow_reset_count[ch] > cfg->shadow_zero_thresh) {
                //printf("checkpoint 5. ych %d\n", ch);
                checkpoints[5] |= 1;
                //# if shadow filter has been reset several times in a row, reset to zeros                
                params->shadow_flag[ch] = ZERO;
                reset_shadow_filter_fp(params, ch);
                for(int i=0; i<NUM_BINS; i++) {
                    params->Error_shadow[ch][i].re = params->Y[ch][i].re;
                    params->Error_shadow[ch][i].im = params->Y[ch][i].im;
                }
                //# give the zeroed filter time to reconverge
                params->shadow_reset_count[ch] = -cfg->shadow_reset_timer;                
            }
            else {
                //printf("checkpoint 6\n");
                checkpoints[6] |= 1;
                //# otherwise copy the main filter to the shadow filter
                main_to_shadow_filter_copy_fp(params, ch);
                for(int i=0; i<NUM_BINS; i++) {
                    params->Error_shadow[ch][i].re = params->Error[ch][i].re;                    
                    params->Error_shadow[ch][i].im = params->Error[ch][i].im;                    
                }
                params->shadow_flag[ch] = RESET;
            }
        }
        else {
            //printf("checkpoint 7\n");
            checkpoints[7] |= 1;
            //# shadow filter is comparable to main filter, 
            //# or we're waiting for it to reconverge after zeroing
            params->shadow_better_count[ch] = 0;
            params->shadow_flag[ch] = EQUAL;
            if(params->shadow_reset_count[ch] < 0) {
                params->shadow_reset_count[ch] += 1;
            }            
        }
    }
    //# reset sigma_xx if both mics shadow filtered
    int both_shadow_filtered = 1;
    for(int ch=0; ch<params->y_channels; ch++) {
        if(params->shadow_flag[ch] <= EQUAL) {
            both_shadow_filtered = 0;
            break;
        }
    }
    if(both_shadow_filtered) {
        //printf("checkpoint 8\n");
        checkpoints[8] |= 1;
        for(int ch=0; ch<params->x_channels; ch++) {
            for(int i=0; i<NUM_BINS; i++) {
                params->sigma_XX[ch][i] = 0.0;
            }
        }
    }
}

void calc_delta_fp(shadow_filt_params_fp_t *params, const coherence_mu_config_fp_t *coh_mu_cfg) {
    if(coh_mu_cfg->adaption_config == AEC_ADAPTION_AUTO) {
        //main filter delta
        double max_energy = params->max_X_energy_main[0];
        for(int ch=1; ch<params->x_channels; ch++) {
            max_energy = (params->max_X_energy_main[ch] > max_energy) ? params->max_X_energy_main[ch] : max_energy;
        }
        max_energy = max_energy * params->delta_scale_main;
        //params->delta_main = (max_energy > params->delta_min) ? max_energy : params->delta_min; 
        if(max_energy > params->delta_min) {
            params->delta_main = max_energy;
        }
        else {
            params->delta_main = params->delta_min;
        }

        //shadow
        max_energy = params->max_X_energy_shadow[0];
        for(int ch=1; ch<params->x_channels; ch++) {
            max_energy = (params->max_X_energy_shadow[ch] > max_energy) ? params->max_X_energy_shadow[ch] : max_energy;
        }
        max_energy = max_energy * params->delta_scale_shadow;
        //params->delta_shadow = (max_energy > params->delta_min) ? max_energy : params->delta_min;
        if(max_energy > params->delta_min) {
            params->delta_shadow = max_energy;
        }
        else {
            params->delta_shadow = params->delta_min;
        }
    }
    else {
        params->delta_main = params->delta_adaption_force_on;
        params->delta_shadow = params->delta_adaption_force_on; 
    }
}

void compare_filters_and_calc_mu_fp(
        shadow_filt_params_fp_t *params,
        const shadow_filt_config_fp_t *shadow_cfg,
        const coherence_mu_config_fp_t *coh_mu_cfg,
        int bypass)
{
    if(bypass) {return;}
    compare_filter_fp(
            params,
            shadow_cfg);
    
    //TODO check if all paths executed
    calc_delta_fp(params, coh_mu_cfg);

    calc_coherence_mu_fp(
            params,
            coh_mu_cfg);

    //update main and shadow filter mu
    for(int ych=0; ych<params->y_channels; ych++) {
        for(int xch=0; xch<params->x_channels; xch++) {
            params->main_filt_mu[ych][xch] = params->coh_mu[ych][xch];
            params->shadow_filt_mu[ych][xch] = shadow_cfg->shadow_mu;
        }
    }
}


void test_compare_filters_and_calc_mu() {
    //What is needed for compare filters
    //Input
    //Y
    //Ov_Error
    //Ov_Error_shadow,
    //Ov_Input
    //ref_energy_thresh
    //ref_energy_log
    //adaption_config
    //force_adaption_mu

    //Output
    //Error
    //Error_shadow
    //H_hat
    //H_hat_shadow
    aec_state_t main_state, shadow_state;
    aec_shared_state_t shared_state;
    aec_memory_pool_t main_memory_pool;
    aec_shadow_filt_memory_pool_t shadow_memory_pool;
    
    unsigned num_y_channels = TEST_NUM_Y;
    unsigned num_x_channels = TEST_NUM_X;
    unsigned main_filter_phases = TEST_MAIN_PHASES;
    unsigned shadow_filter_phases = TEST_SHADOW_PHASES;

    //floating point arrays
    shadow_filt_config_fp_t shadow_filt_cfg_fp;
    coherence_mu_config_fp_t coh_mu_cfg_fp;
    shadow_filt_params_fp_t shadow_filt_coh_mu_params_fp; 

    //Initialise fixed point
    aec_init(&main_state, &shadow_state, &shared_state, (uint8_t*)&main_memory_pool, (uint8_t*)&shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);
    //Initialise floating point
    init_shadow_config_fp(&shadow_filt_cfg_fp);
    init_coherence_mu_config_fp(&coh_mu_cfg_fp);
    init_params_fp(
            &shadow_filt_coh_mu_params_fp,
            &shadow_filt_cfg_fp,
            &coh_mu_cfg_fp,
            num_y_channels,
            num_x_channels,
            main_filter_phases,
            shadow_filter_phases);
    
    unsigned seed = 35788;
    int32_t new_frame[TEST_NUM_Y + TEST_NUM_X][AEC_FRAME_ADVANCE];
    unsigned max_diff_coh_mu = 0; 
    for(int iter=0; iter<(1<<11)/F; iter++) {
        //every 200 frames set bypass
        main_state.shared_state->config_params.aec_core_conf.bypass = 0;
        if((iter > 0) && !(iter % 200)) {
            main_state.shared_state->config_params.aec_core_conf.bypass = pseudo_rand_uint32(&seed) % 2;
        }
        //printf("iter %d\n",iter);
        aec_frame_init(&main_state, &shadow_state, &new_frame[0], &new_frame[TEST_NUM_Y]);
        for(int ch=0; ch<num_y_channels; ch++) {
            //state_ptr->shared_state->Y is initialised in the y->Y fft aec_fft() call with state_ptr->shared_state->y as input. Initialising here for
            //standalone testing.
            bfp_complex_s32_init(&main_state.shared_state->Y[ch], (complex_s32_t*)&main_state.shared_state->y[ch].data[0], 0, NUM_BINS, 0);
        }

        shadow_filt_params_fp_t *params_fp = &shadow_filt_coh_mu_params_fp;
        //generate input

        for(int ych=0; ych<num_y_channels; ych++) {
            //H_hat
            for(int ph=0; ph<num_x_channels*main_state.num_phases; ph++) {
                int xch = ph/main_state.num_phases;
                int ph_xch = (main_state.num_phases == 1) ? 0 : ph % main_state.num_phases;
                main_state.H_hat[ych][ph].exp = pseudo_rand_int(&seed, -31, 32);
                main_state.H_hat[ych][ph].hr = pseudo_rand_uint32(&seed) % 3;
                for(int i=0; i<NUM_BINS; i++) {
                    main_state.H_hat[ych][ph].data[i].re = pseudo_rand_int32(&seed) >> main_state.H_hat[ych][ph].hr;
                    main_state.H_hat[ych][ph].data[i].im = pseudo_rand_int32(&seed) >> main_state.H_hat[ych][ph].hr;
                    params_fp->H_hat[ych][xch][ph_xch][i].re = ldexp(main_state.H_hat[ych][ph].data[i].re, main_state.H_hat[ych][ph].exp);
                    params_fp->H_hat[ych][xch][ph_xch][i].im = ldexp(main_state.H_hat[ych][ph].data[i].im, main_state.H_hat[ych][ph].exp);
                }
            }
            for(int ph=0; ph<num_x_channels*shadow_state.num_phases; ph++) {
                int xch = ph/shadow_state.num_phases;
                int ph_xch = (shadow_state.num_phases == 1) ? 0 : ph % shadow_state.num_phases; //phase within the given xch
                shadow_state.H_hat[ych][ph].exp = pseudo_rand_int(&seed, -31, 32);
                shadow_state.H_hat[ych][ph].hr = pseudo_rand_uint32(&seed) % 3;
                for(int i=0; i<NUM_BINS; i++) {
                    shadow_state.H_hat[ych][ph].data[i].re = pseudo_rand_int32(&seed) >> shadow_state.H_hat[ych][ph].hr;
                    shadow_state.H_hat[ych][ph].data[i].im = pseudo_rand_int32(&seed) >> shadow_state.H_hat[ych][ph].hr;
                    params_fp->H_hat_shadow[ych][xch][ph_xch][i].re = ldexp(shadow_state.H_hat[ych][ph].data[i].re, shadow_state.H_hat[ych][ph].exp);
                    params_fp->H_hat_shadow[ych][xch][ph_xch][i].im = ldexp(shadow_state.H_hat[ych][ph].data[i].im, shadow_state.H_hat[ych][ph].exp);
                }
            }
            //Error, Error_shadow, Y
            main_state.Error[ych].exp = pseudo_rand_int(&seed, -31, 32);
            main_state.Error[ych].hr = pseudo_rand_uint32(&seed) % 3;
            shadow_state.Error[ych].exp = pseudo_rand_int(&seed, -31, 32);
            shadow_state.Error[ych].hr = pseudo_rand_uint32(&seed) % 3;
            main_state.shared_state->Y[ych].exp = pseudo_rand_int(&seed, -31, 32);
            main_state.shared_state->Y[ych].hr = pseudo_rand_uint32(&seed) % 3;
            for(int i=0; i<NUM_BINS; i++) {
                //Error
                main_state.Error[ych].data[i].re = pseudo_rand_int32(&seed) >> main_state.Error[ych].hr;
                main_state.Error[ych].data[i].im = pseudo_rand_int32(&seed) >> main_state.Error[ych].hr;
                params_fp->Error[ych][i].re = ldexp(main_state.Error[ych].data[i].re, main_state.Error[ych].exp);
                params_fp->Error[ych][i].im = ldexp(main_state.Error[ych].data[i].im, main_state.Error[ych].exp);

                //Error_shadow
                shadow_state.Error[ych].data[i].re = pseudo_rand_int32(&seed) >> shadow_state.Error[ych].hr;
                shadow_state.Error[ych].data[i].im = pseudo_rand_int32(&seed) >> shadow_state.Error[ych].hr;
                params_fp->Error_shadow[ych][i].re = ldexp(shadow_state.Error[ych].data[i].re, shadow_state.Error[ych].exp);
                params_fp->Error_shadow[ych][i].im = ldexp(shadow_state.Error[ych].data[i].im, shadow_state.Error[ych].exp);

                //Y
                main_state.shared_state->Y[ych].data[i].re = pseudo_rand_int32(&seed) >> main_state.shared_state->Y[ych].hr;
                main_state.shared_state->Y[ych].data[i].im = pseudo_rand_int32(&seed) >> main_state.shared_state->Y[ych].hr;
                params_fp->Y[ych][i].re = ldexp(main_state.shared_state->Y[ych].data[i].re, main_state.shared_state->Y[ych].exp);
                params_fp->Y[ych][i].im = ldexp(main_state.shared_state->Y[ych].data[i].im, main_state.shared_state->Y[ych].exp);
            }
            //overall_Error
            main_state.overall_Error[ych].exp = pseudo_rand_int(&seed, -31, 32);
            main_state.overall_Error[ych].mant = pseudo_rand_uint32(&seed) >> 1;
            params_fp->overall_Error[ych] = ldexp(main_state.overall_Error[ych].mant, main_state.overall_Error[ych].exp);
            
            //overall_Error_shadow
            shadow_state.overall_Error[ych].exp = pseudo_rand_int(&seed, -31, 32);
            shadow_state.overall_Error[ych].mant = pseudo_rand_uint32(&seed) >> 1;
            params_fp->overall_Error_shadow[ych] = ldexp(shadow_state.overall_Error[ych].mant, shadow_state.overall_Error[ych].exp);
            
            //overall_Y
            main_state.shared_state->overall_Y[ych].exp = pseudo_rand_int(&seed, -31, 32); 
            main_state.shared_state->overall_Y[ych].mant = pseudo_rand_uint32(&seed) >> 1;
            params_fp->overall_Y[ych] = ldexp(main_state.shared_state->overall_Y[ych].mant, main_state.shared_state->overall_Y[ych].exp);

            //shadow_reset_count
            main_state.shared_state->shadow_filter_params.shadow_reset_count[ych] = pseudo_rand_int(&seed, -31, 32);
            params_fp->shadow_reset_count[ych] = main_state.shared_state->shadow_filter_params.shadow_reset_count[ych];
            //shadow_better_count
            main_state.shared_state->shadow_filter_params.shadow_better_count[ych] = pseudo_rand_uint32(&seed)%8; //between 0 and 7
            params_fp->shadow_better_count[ych] = main_state.shared_state->shadow_filter_params.shadow_better_count[ych];
            //coh
            main_state.shared_state->coh_mu_state[ych].coh.exp = pseudo_rand_int(&seed, -3, 4) - 31;
            main_state.shared_state->coh_mu_state[ych].coh.mant = pseudo_rand_uint32(&seed) & 0x7fffffff;
            params_fp->coh[ych] = ldexp(main_state.shared_state->coh_mu_state[ych].coh.mant, main_state.shared_state->coh_mu_state[ych].coh.exp);
            //coh_slow
            main_state.shared_state->coh_mu_state[ych].coh_slow.exp = pseudo_rand_int(&seed, -3, 4) - 31;
            main_state.shared_state->coh_mu_state[ych].coh_slow.mant = pseudo_rand_uint32(&seed) & 0x7fffffff;
            params_fp->coh_slow[ych] = ldexp(main_state.shared_state->coh_mu_state[ych].coh_slow.mant, main_state.shared_state->coh_mu_state[ych].coh_slow.exp);

            //clear mu_coh_count and mu_shad_count every few frames for better code coverage in calc_coherence_mu_fp
            if((iter > 0) && !(iter % (params_fp->mu_shad_count[ych] + 2))) {
                main_state.shared_state->coh_mu_state[ych].mu_coh_count = 0;
                params_fp->mu_coh_count[ych] = 0;
                main_state.shared_state->coh_mu_state[ych].mu_shad_count = 0;
                params_fp->mu_shad_count[ych] = 0;
            }
        }
        
        //Set adaption_config to something other than AUTO once in a while
        if((iter > 0) && !(iter % 100)) {
            int force_on = pseudo_rand_uint32(&seed) % 2;
            if(force_on) {
                main_state.shared_state->config_params.coh_mu_conf.adaption_config = AEC_ADAPTION_FORCE_ON;
                coh_mu_cfg_fp.adaption_config = AEC_ADAPTION_FORCE_ON;
            }
            else {
                main_state.shared_state->config_params.coh_mu_conf.adaption_config = AEC_ADAPTION_FORCE_OFF;
                coh_mu_cfg_fp.adaption_config = AEC_ADAPTION_FORCE_OFF;
            }
            main_state.shared_state->config_params.coh_mu_conf.force_adaption_mu_q30 = pseudo_rand_uint32(&seed) & 0x7fffffff;
            coh_mu_cfg_fp.force_adaption_mu = ldexp(main_state.shared_state->config_params.coh_mu_conf.force_adaption_mu_q30, -30);
        }
        if((iter > 0) && !(iter % 15)) {
            double min_coh_slow = params_fp->coh_slow[0];
            for(int ch=1; ch<params_fp->y_channels; ch++) {
                if(params_fp->coh_slow[ch] < min_coh_slow) min_coh_slow = params_fp->coh_slow[ch];
            }
            double CC_thres = min_coh_slow * coh_mu_cfg_fp.coh_thresh_slow;
            //set coh to a number between CC_thres and coh_slow for code coverage of an if condition
            for(int ch=0; ch<params_fp->y_channels; ch++) {
                params_fp->coh[ch] = CC_thres + (params_fp->coh_slow[ch] - CC_thres)/(3.15 + params_fp->coh[ch]);
                main_state.shared_state->coh_mu_state[ch].coh = f64_to_float_s32(params_fp->coh[ch]);
            }
        }
        //mu_scalar
        main_state.shared_state->config_params.coh_mu_conf.mu_scalar.exp = pseudo_rand_int(&seed, -3, 4) - 31;
        main_state.shared_state->config_params.coh_mu_conf.mu_scalar.mant = pseudo_rand_uint32(&seed) & 0x7fffffff;
        coh_mu_cfg_fp.mu_scalar = ldexp(main_state.shared_state->config_params.coh_mu_conf.mu_scalar.mant, main_state.shared_state->config_params.coh_mu_conf.mu_scalar.exp);

        for(int xch=0; xch<num_x_channels; xch++) {
            //sigma_XX
            main_state.shared_state->sigma_XX[xch].exp = pseudo_rand_int(&seed, -31, 32);
            main_state.shared_state->sigma_XX[xch].hr = pseudo_rand_uint32(&seed) % 3;
            for(int i=0; i<NUM_BINS; i++) {
                main_state.shared_state->sigma_XX[xch].data[i] = pseudo_rand_int32(&seed) >> main_state.shared_state->sigma_XX[xch].hr;
                params_fp->sigma_XX[xch][i] = ldexp(main_state.shared_state->sigma_XX[xch].data[i], main_state.shared_state->sigma_XX[xch].exp);
            }
            
            //sum_X_energy
            main_state.shared_state->sum_X_energy[xch].exp = pseudo_rand_int(&seed, -31, 32) - 31;
            main_state.shared_state->sum_X_energy[xch].mant = pseudo_rand_uint32(&seed) >> 1;
            params_fp->sum_X_energy[xch] = ldexp(main_state.shared_state->sum_X_energy[xch].mant, main_state.shared_state->sum_X_energy[xch].exp);

            //max_X_energy
            main_state.max_X_energy[xch].exp = -32 - (pseudo_rand_uint32(&seed) & 63);
            main_state.max_X_energy[xch].mant = pseudo_rand_uint32(&seed) >> 1;
            params_fp->max_X_energy_main[xch] = ldexp(main_state.max_X_energy[xch].mant, main_state.max_X_energy[xch].exp);
            //Make shadow max_energy smaller since shadow scale is bigger and we want the scaled_max_energy < delta_min code path executed
            shadow_state.max_X_energy[xch].exp = -40 - (pseudo_rand_uint32(&seed) & 63); 
            shadow_state.max_X_energy[xch].mant = pseudo_rand_uint32(&seed) >> 1;
            params_fp->max_X_energy_shadow[xch] = ldexp(shadow_state.max_X_energy[xch].mant, shadow_state.max_X_energy[xch].exp);

        }
        
        compare_filters_and_calc_mu_fp(
                &shadow_filt_coh_mu_params_fp,
                &shadow_filt_cfg_fp,
                &coh_mu_cfg_fp,
                main_state.shared_state->config_params.aec_core_conf.bypass);

        aec_compare_filters_and_calc_mu(
                &main_state,
                &shadow_state);

        //check compare_filters outputs
        for(int ych=0; ych<num_y_channels; ych++) {
            //compare shadow_flag
            if(params_fp->shadow_flag[ych] != main_state.shared_state->shadow_filter_params.shadow_flag[ych]) {
                printf("iter %d. shadow_flag (ref %d, dut %ld) error\n", iter, params_fp->shadow_flag[ych], main_state.shared_state->shadow_filter_params.shadow_flag[ych]);
                assert(0);
            }

            //compare shadow_reset_count
            if(params_fp->shadow_reset_count[ych] != main_state.shared_state->shadow_filter_params.shadow_reset_count[ych]) {
                printf("iter %d. shadow_reset_count (ref %d, dut %d) error\n", iter, params_fp->shadow_reset_count[ych], main_state.shared_state->shadow_filter_params.shadow_reset_count[ych]);
                assert(0);
            }

            //compare shadow_better_count
            if(params_fp->shadow_better_count[ych] != main_state.shared_state->shadow_filter_params.shadow_better_count[ych]) {
                printf("iter %d. shadow_better_count (ref %d, dut %d) error\n", iter, params_fp->shadow_better_count[ych], main_state.shared_state->shadow_filter_params.shadow_better_count[ych]);
                assert(0);
            }
            //compare Error
            unsigned diff_Error = vector_int32_maxdiff((int32_t*)&main_state.Error[ych].data[0], main_state.Error[ych].exp, (double*)params_fp->Error[ych], 0, 2*NUM_BINS);
            if(diff_Error > 0) {printf("iter %d. diff_Error %d too large\n",iter, diff_Error); assert(0);}

            unsigned diff_Error_shadow = vector_int32_maxdiff((int32_t*)&shadow_state.Error[ych].data[0], shadow_state.Error[ych].exp, (double*)params_fp->Error_shadow[ych], 0, 2*NUM_BINS);

            //compare Error_Shadow
            if(diff_Error_shadow > 0) {printf("iter %d. diff_Error_shadow %d too large\n",iter, diff_Error_shadow); assert(0);}
            
            //Compare H_hat and H_hat_shadow
            for(int xch=0; xch<num_x_channels; xch++) {
                for(int ph=0; ph<main_state.num_phases; ph++) {
                    unsigned diff_H_hat = vector_int32_maxdiff((int32_t*)&main_state.H_hat[ych][xch*main_state.num_phases + ph].data[0], main_state.H_hat[ych][xch*main_state.num_phases + ph].exp, (double*)&params_fp->H_hat[ych][xch][ph][0], 0, 2*NUM_BINS);
                    if(diff_H_hat > 0){printf("iter %d, ych %d, xch %d, ph %d, shadow_flag %d. diff_H_hat %d too large\n",iter, ych, xch, ph, params_fp->shadow_flag[ych], diff_H_hat); assert(0);}
                }
                
                for(int ph=0; ph<shadow_state.num_phases; ph++) {
                    unsigned diff_H_hat_shadow = vector_int32_maxdiff((int32_t*)&shadow_state.H_hat[ych][xch*shadow_state.num_phases + ph].data[0], shadow_state.H_hat[ych][xch*shadow_state.num_phases + ph].exp, (double*)&params_fp->H_hat_shadow[ych][xch][ph][0], 0, 2*NUM_BINS);
                    if(diff_H_hat_shadow > 0){printf("iter %d, ych %d, xch %d, ph %d, shadow_flag %d. diff_H_hat_shadow %d too large\n",iter, ych, xch, ph, params_fp->shadow_flag[ych], diff_H_hat_shadow); assert(0);}
                }
            }
        }

        //compare sigma_XX
        for(int xch=0; xch<num_x_channels; xch++) {
            unsigned diff_sigma_XX = vector_int32_maxdiff((int32_t*)&main_state.shared_state->sigma_XX[xch].data[0], main_state.shared_state->sigma_XX[xch].exp, &params_fp->sigma_XX[xch][0], 0, NUM_BINS);
            if(diff_sigma_XX > 0) {printf("iter %d. diff_sigma_XX %d too large\n",iter, diff_sigma_XX); assert(0);}
        }

        //compare delta
        unsigned delta_diff = vector_int32_maxdiff((int32_t*)&main_state.delta.mant, main_state.delta.exp, &params_fp->delta_main, 0, 1);
        if(delta_diff > 1) {printf("iter %d. delta_diff_main %d too large\n",iter, delta_diff); assert(0);}
        delta_diff = vector_int32_maxdiff((int32_t*)&shadow_state.delta.mant, shadow_state.delta.exp, &params_fp->delta_shadow, 0, 1);
        if(delta_diff > 1) {printf("iter %d. delta_diff_shadow %d too large\n",iter, delta_diff); assert(0);}

        
        //check calc_mu outputs
        for(int ych=0; ych<num_y_channels; ych++) {

            //compare mu_coh_count
            if(params_fp->mu_coh_count[ych] != main_state.shared_state->coh_mu_state[ych].mu_coh_count) {
                printf("iter %d. mu_coh_count mismatch. (ref %d, dut %ld)\n",iter, params_fp->mu_coh_count[ych], main_state.shared_state->coh_mu_state[ych].mu_coh_count); assert(0);}
            
            //compare mu_shad_count
            if(params_fp->mu_shad_count[ych] != main_state.shared_state->coh_mu_state[ych].mu_shad_count) {printf("iter %d. mu_shad_count mismatch. (ref %d, dut %ld\n",iter, params_fp->mu_shad_count[ych], main_state.shared_state->coh_mu_state[ych].mu_shad_count); assert(0);}
            
            //compare coh_mu
            for(int xch=0; xch<num_x_channels; xch++) {
                unsigned diff_coh_mu = vector_int32_maxdiff((int32_t*)&main_state.shared_state->coh_mu_state[ych].coh_mu[xch].mant, main_state.shared_state->coh_mu_state[ych].coh_mu[xch].exp, (double*)&params_fp->coh_mu[ych][xch], 0, 1);
                if(diff_coh_mu > 64) {
                    printf("iter %d. diff_coh_mu[%d][%d] %d too large\n",iter, ych, xch, diff_coh_mu);
                    assert(0);
                }
                if(diff_coh_mu > max_diff_coh_mu) {max_diff_coh_mu = diff_coh_mu;}
            }

            //Finally compare main and shadow filter mu
            for(int xch=0; xch<num_x_channels; xch++) {
                unsigned diff_main_filt_mu = vector_int32_maxdiff((int32_t*)&main_state.mu[ych][xch].mant, main_state.mu[ych][xch].exp, (double*)&params_fp->main_filt_mu[ych][xch], 0, 1);
                if(diff_main_filt_mu > 64) {
                    printf("iter %d. diff_main_filt_mu[%d][%d] %d too large\n",iter, ych, xch, diff_main_filt_mu);
                    assert(0);
                }
            }
            for(int xch=0; xch<num_x_channels; xch++) {
                unsigned diff_shadow_filt_mu = vector_int32_maxdiff((int32_t*)&shadow_state.mu[ych][xch].mant, shadow_state.mu[ych][xch].exp, (double*)&params_fp->shadow_filt_mu[ych][xch], 0, 1);
                if(diff_shadow_filt_mu > 64) {
                    printf("iter %d. diff_shadow_filt_mu[%d][%d] %d too large\n",iter, ych, xch, diff_shadow_filt_mu);
                    assert(0);
                }
            }
        }
    }
    for(int i=0; i<NUM_SHADOW_CHECKPOINTS; i++) {
        if(checkpoints[i] != 1) {printf("checkpoint %d not tested\n",i); assert(0);}
        //printf("checkpoints[%d] = %d\n",i, checkpoints[i]);
    }
    printf("max_diff_coh_mu = %d\n",max_diff_coh_mu);
    for(int i=0; i<NUM_MU_CHECKPOINTS; i++) {
        if((checkpoints_mu[i] != 1) && ( i!=7 )) {printf("checkpoint_mu %d not tested\n",i); assert(0);}
        //printf("checkpoints_mu[%d] = %d\n",i, checkpoints_mu[i]);
    }

}
