#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_api.h"

#define TEST_MAIN_PHASES (10)
#define TEST_NUM_Y (2)
#define TEST_NUM_X (2)
#define TEST_SHADOW_PHASES (0)
#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)

double calc_corr_factor_fp(double *y_full, double *yhat_full) {
    double *y = &y_full[AEC_FRAME_ADVANCE];
    double *yhat = &yhat_full[AEC_FRAME_ADVANCE];
    
    double y_abs[AEC_FRAME_ADVANCE], yhat_abs[AEC_FRAME_ADVANCE];
    for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
        y_abs[i] = (y[i] < 0.0) ? -y[i] : y[i];
        yhat_abs[i] = (yhat[i] < 0.0) ? -yhat[i] : yhat[i];
    }

    double sigma_yyhat = 0.0;
    double sigma_absy_absyhat = 0.0;
    for(int i=0; i<AEC_FRAME_ADVANCE-32; i++) {
        sigma_yyhat += (y[i] * yhat[i]);
        sigma_absy_absyhat += (y_abs[i] * yhat_abs[i]);
    }
    if(sigma_yyhat < 0.0) {sigma_yyhat = -sigma_yyhat;}
    if(sigma_absy_absyhat == 0.0) {return 0.0;}
    double div = sigma_yyhat/sigma_absy_absyhat; 
    return div;
}

void test_calc_corr_factor() {
    unsigned num_y_channels = TEST_NUM_Y;
    unsigned num_x_channels = TEST_NUM_X;
    unsigned main_filter_phases = TEST_MAIN_PHASES;
    unsigned shadow_filter_phases = TEST_SHADOW_PHASES;
    
    aec_state_t state;
    aec_memory_pool_t DWORD_ALIGNED aec_memory_pool;
    aec_shared_state_t aec_shared_state;
    double y_fp[TEST_NUM_Y][AEC_PROC_FRAME_LENGTH], y_hat_fp[TEST_NUM_Y][AEC_PROC_FRAME_LENGTH];
    
    aec_init(&state, NULL, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)NULL, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);
    
    unsigned seed = 10345;
    int32_t max_diff = 0; 
    for(int iter=0; iter<(1<<12)/F; iter++) {
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

        for(int ch=0; ch<num_y_channels; ch++) {
            double ref_corr = calc_corr_factor_fp(y_fp[ch], y_hat_fp[ch]);
            float_s32_t dut_corr = aec_calc_corr_factor(&state, ch); 
            int32_t ref = double_to_int32(ref_corr, dut_corr.exp);
            int32_t diff = abs(ref - dut_corr.mant);
            TEST_ASSERT_LESS_OR_EQUAL_INT32_MESSAGE(1<<15, diff, "corr_factor diff too large.");
            if(diff > max_diff) {max_diff = diff;}
        } 
    }
    //printf("max_diff = %d\n",max_diff);
}
