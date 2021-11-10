// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved
#ifndef IC_STATE_H_
#define IC_STATE_H_

#include <xccompat.h>
#include "dsp.h"
#include "vtb_q_format.h"

#define IC_DEBUG_PRINT 0

#ifndef IC_FRAME_ADVANCE
#error IC_FRAME_ADVANCE has not been set
#endif

#ifndef IC_INPUT_CHANNELS
#error IC_INPUT_CHANNELS has not been set
#endif

#ifndef IC_PROC_FRAME_LENGTH_LOG2
#error IC_PROC_FRAME_LENGTH_LOG2 has not been set
#endif
#if IC_PROC_FRAME_LENGTH_LOG2 == 6
#define IC_FFT_SINE_LUT dsp_sine_64
#define IC_FFT_SINE_LUT_HALF dsp_sine_32
#elif  IC_PROC_FRAME_LENGTH_LOG2 == 7
#define IC_FFT_SINE_LUT dsp_sine_128
#define IC_FFT_SINE_LUT_HALF dsp_sine_64
#elif  IC_PROC_FRAME_LENGTH_LOG2 == 8
#define IC_FFT_SINE_LUT dsp_sine_256
#define IC_FFT_SINE_LUT_HALF dsp_sine_128
#elif  IC_PROC_FRAME_LENGTH_LOG2 == 9
#define IC_FFT_SINE_LUT dsp_sine_512
#define IC_FFT_SINE_LUT_HALF dsp_sine_256
#elif  IC_PROC_FRAME_LENGTH_LOG2 == 10
#define IC_FFT_SINE_LUT dsp_sine_1024
#define IC_FFT_SINE_LUT_HALF dsp_sine_512
#endif

#ifndef IC_INPUT_CHANNELS
#define IC_INPUT_CHANNELS     2
#elif IC_INPUT_CHANNELS != 2
#error "IC_INPUT_CHANNELS must be 2"
#endif

#ifndef IC_OUTPUT_CHANNELS
#define IC_OUTPUT_CHANNELS     2
#endif

#define IC_INPUT_CHANNEL_PAIRS    ((IC_INPUT_CHANNELS+1)/2)
#define IC_OUTPUT_CHANNEL_PAIRS    ((IC_OUTPUT_CHANNELS+1)/2)
#define IC_PROC_FRAME_LENGTH      (1<<IC_PROC_FRAME_LENGTH_LOG2)
#define IC_PROC_FRAME_BINS_LOG2   (IC_PROC_FRAME_LENGTH_LOG2 - 1)
#define IC_PROC_FRAME_BINS        (1<<IC_PROC_FRAME_BINS_LOG2)


//This denotes which of the channels is the X and which is the Y in the case of LMS
//i.e. output = Y - X*H
//Note: in order for vtb_form_output_frame to work Y must be first.
#define IC_X_CHANNEL 1
#define IC_Y_CHANNEL 0

#define IC_ERROR_MIN_HEADROOM (IC_PROC_FRAME_LENGTH_LOG2)
#define IC_H_HAT_MIN_HEADROOM 6 // ceil(log2(IC_PHASES)) it's one more then i think due to addition in the complex mul.

#define IC_PROC_FRAME_LENGTH (1<<IC_PROC_FRAME_LENGTH_LOG2)

#define IC_UNUSED_TAPS_PER_PHASE ((IC_PROC_FRAME_LENGTH - (2*IC_FRAME_ADVANCE))/2)

#define IC_TOTAL_INPUT_CHANNELS (IC_INPUT_CHANNELS + IC_PASSTHROUGH_CHANNELS)
#define IC_TOTAL_INPUT_CHANNEL_PAIRS ((IC_TOTAL_INPUT_CHANNELS + 1)/2)

#define IC_TOTAL_OUTPUT_CHANNELS (IC_OUTPUT_CHANNELS + IC_PASSTHROUGH_CHANNELS)
#define IC_TOTAL_OUTPUT_CHANNEL_PAIRS ((IC_TOTAL_OUTPUT_CHANNELS + 1)/2)

typedef enum {
    IC_ADAPTION_FORCE_ON,
    IC_ADAPTION_FORCE_OFF,
} ic_adaption_e;

typedef struct {
    dsp_complex_t Error[IC_PROC_FRAME_BINS];
    int Error_exp;
    unsigned Error_headroom;
}
#ifdef __XC__
[[aligned(8)]] ic_error_t;
#else
ic_error_t __attribute__ ((aligned(8)));
#endif 

typedef struct ic_state_t{
    uint64_t alignment_padding;
    dsp_complex_t X_fifo[IC_PHASES][IC_PROC_FRAME_BINS];
    dsp_complex_t H_hat[IC_PHASES][IC_PROC_FRAME_BINS];
    vtb_ch_pair_t output[IC_TOTAL_OUTPUT_CHANNEL_PAIRS][IC_FRAME_ADVANCE] ;//?

    uint32_t X_energy[IC_PROC_FRAME_BINS];
    uint32_t sigma_xx[IC_PROC_FRAME_BINS];

    vtb_ch_pair_t overlap[IC_UNUSED_TAPS_PER_PHASE*2];

    int H_hat_exp[IC_PHASES];
    unsigned H_hat_hr[IC_PHASES];

    unsigned X_fifo_index;          // Pointer to the head of the X fifo.
    unsigned X_fifo_hr[IC_PHASES];
    int X_fifo_exp[IC_PHASES];

    unsigned X_fifo_step; //let's us know where we are going to insert the next X frame

    unsigned x_channels[1];
    unsigned x_channel_phase_counts[1];
    unsigned mapping[IC_PHASES];
    unsigned ch_head[1];
    unsigned phase_to_ch[IC_PHASES];

    int X_energy_exp; //This is the exponent of X_energy
    int sigma_xx_exp; //This is the exponent of sigma_xx
    unsigned sigma_xx_hr;
    unsigned X_energy_hr;

    unsigned X_energy_recalc_bin;

    ic_adaption_e adaption_config;
    vtb_uq1_31_t force_adaption_mu;
    uint32_t sigma_xx_shift;
    uint32_t delta;
    int delta_exp;
    int gamma_log2;//controls the amount of sigma in inv_x_energy


    vtb_q0_31_t leakage_alpha; //sets the leakage on H_hat
    int32_t use_noise_minimisation;
    int bypass;
    uint32_t coeff_index;
    unsigned x_channel_delay;
    uint8_t output_beamform_on_ch1;
} ic_state_t;

void ic_test_task(chanend app_to_ic, chanend ic_to_app, NULLABLE_RESOURCE(chanend, c_control));

void ic_dump_paramters(REFERENCE_PARAM(ic_state_t, state));

void ic_init(REFERENCE_PARAM(ic_state_t, state));

void ic_reset(REFERENCE_PARAM(ic_state_t, state));

void ic_process_td_frame(REFERENCE_PARAM(ic_state_t, state),
        vtb_ch_pair_t frame[IC_TOTAL_INPUT_CHANNEL_PAIRS][IC_PROC_FRAME_LENGTH],
        int channel_hr[IC_TOTAL_INPUT_CHANNEL_PAIRS*2],
        REFERENCE_PARAM(int, Error_exp));

void ic_adapt_filter(REFERENCE_PARAM(ic_state_t, state),
        vtb_ch_pair_t frame[IC_TOTAL_OUTPUT_CHANNEL_PAIRS][IC_PROC_FRAME_LENGTH],
        int Error_exp);

//TODO add control for C
#ifdef __XC__
#include "ic_control.h"
#endif

#endif // IC_STATE_H_
