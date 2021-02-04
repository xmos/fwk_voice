// Copyright (c) 2017-2019, XMOS Ltd, All rights reserved
#ifndef _VOICE_TOOLBOX_FP_H_
#define _VOICE_TOOLBOX_FP_H_

#include <dsp.h>
#include <vtb_ch_pair.h>

typedef struct {
    // Chunked state
    unsigned chunk_size;
    unsigned sample_index;

    // Parameters
    unsigned frame_advance;
    unsigned num_ch;
} vtb_tx_state_fp;

typedef struct {
    // Chunked state
    unsigned chunk_size;
    unsigned sample_index;

    // Buffers
    vtb_ch_pair_fp * unsafe input_frame; // FRAME_ADVANCE
    vtb_ch_pair_fp * unsafe prev_frame;  // PROC_FRAME_LENGTH - FRAME_ADVANCE
    double * unsafe delay_buffer;      // sum(delays)

    // Parameters
    unsigned frame_advance;
    unsigned proc_frame_length;
    unsigned num_ch;
    unsigned * unsafe delays; // Array[num_ch] = {delay in samples for each channel}
    unsigned delayed; // Set to 1 in form_rx_state if sum(delays) != 0
} vtb_rx_state_fp;


void vtb_fd_hanning_fp(dsp_complex_fp * d, unsigned length);

void vtb_abs_channel_fp(double * x_fp, dsp_complex_fp * X, unsigned length);

void vtb_form_processing_frame_fp(vtb_ch_pair_fp *proc_frame,
        vtb_ch_pair_fp *input_frame,
        vtb_ch_pair_fp *prev_frame,
        const unsigned frame_advance,
        const unsigned proc_frame_length,
        const unsigned num_ch);

void vtb_adapt_fp(dsp_complex_fp * unsafe H,
        const dsp_complex_fp * unsafe T,
        const dsp_complex_fp * unsafe X,
        const unsigned bin_start, const unsigned bin_stop);

void vtb_adapt_scaled_fp(dsp_complex_fp * unsafe H,
        const dsp_complex_fp * unsafe T,
        const dsp_complex_fp * unsafe X,
        const unsigned bin_start, const unsigned bin_stop, double scalar);

void vtb_compute_Error_fp(
        dsp_complex_fp * unsafe X_fifo,
        dsp_complex_fp * unsafe H_hat,
        dsp_complex_fp * unsafe Y,
        unsigned num_bins,
        unsigned num_phases,
        unsigned td_processing_frame_size,
        unsigned * mapping);

void vtb_compute_T_fp(
        dsp_complex_fp * unsafe T,
        dsp_complex_fp * unsafe Error,
        double * unsafe X_energy_inv,
        double mu,
        unsigned count);

void vtb_compute_T_vector_mu_fp(
        dsp_complex_fp * unsafe T,
        dsp_complex_fp * unsafe Error,
        double * unsafe X_energy_inv,
        double * unsafe mu,
        unsigned count);

void vtb_scale_complex_fp(dsp_complex_fp * d, unsigned count, double scalar);

double vtb_get_fd_frame_power_fp(dsp_complex_fp * X_fp,
            unsigned bin_count);

double vtb_get_td_frame_power_fp(vtb_ch_pair_fp * x,
            unsigned frame_length,
            int select_ch_b);
double vtb_scale_and_limit_mu_fp(double mu,
            double mu_high,
            double mu_low,
            double mu_scalar);

void vtb_save_input_to_output_fp(vtb_ch_pair_fp * output_pair,
        vtb_ch_pair_fp * input_pair,
            unsigned count,
            int input_ch_b,
            int ouput_ch_b);

void vtb_save_input_to_output_pair_fp(
        vtb_ch_pair_fp * unsafe output_pair,
        vtb_ch_pair_fp * unsafe input_pair,
        unsigned count);

void vtb_form_output_frame_fp(
        vtb_ch_pair_fp * unsafe data, //[AEC_PROC_FRAME_LENGTH]
        vtb_ch_pair_fp * unsafe overlap,//[AEC_UNUSED_TAPS_PER_PHASE*2],
        vtb_ch_pair_fp * unsafe output_frame,//[AEC_FRAME_ADVANCE]
        int compute_second_channel,
        unsigned N,
        unsigned unused_taps_per_phase
        );

void vtb_steer_fp(double samples,
        double scalar,
            dsp_complex_fp * data,
            unsigned length,
            int shift_left);

void vtb_zero_error_fp(vtb_ch_pair_fp * unsafe error, unsigned count);

void vtb_sum_energy_fp(double * unsafe denom,
        double * unsafe X_energy,
        double * unsafe sigma_xx, double gamma,
        double delta,  unsigned count);

void vtb_inv_X_energy_fp(double * unsafe inv_X_energy, unsigned count);

double vtb_recalc_energy_fp(dsp_complex_fp * unsafe X,
        unsigned phases, unsigned bins, unsigned offset);

void vtb_add_energy_fp(double * unsafe X_energy,
        dsp_complex_fp * unsafe X, unsigned count);

void vtb_sub_energy_fp(double * unsafe X_energy,
        dsp_complex_fp * unsafe X, unsigned count);

void vtb_update_sigma_fp(double * unsafe sigma_xx,
        dsp_complex_fp * unsafe X, double sigma_alpha, unsigned count);

void vtb_exponential_average_fp(double &x, double new_sample, double alpha);

void vtb_reduce_magnitude_fp(dsp_complex_fp &Y, double u, double d);

#endif /* _VOICE_TOOLBOX_FP_H_ */
