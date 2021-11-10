// Copyright (c) 2019-2020, XMOS Ltd, All rights reserved
#ifndef _VTB_ADAPTIVE_FILTERING_H_
#define _VTB_ADAPTIVE_FILTERING_H_

#include <stdint.h>
#include "dsp.h"
#include "vtb_ch_pair.h"

#ifdef __XC__
/**
 * Helper function to compute the new exponent after compute_T_*
 */
int vtb_compute_T_new_exp(int Error_exp, unsigned Error_hr, int mu_exp, int X_energy_inv_exp);

/**
 *  This computes the product of Error, mu and 1/x_energy.
 *  The result is stored in the Error array as it is not redundant.
 *  mu is a per channel value and 1/X_energy is a per bin per channel
 *  value.
 */
void vtb_compute_T_asm(dsp_complex_t * unsafe T,
        dsp_complex_t * unsafe Error,
        uint32_t * unsafe X_energy_inv,
        uint32_t mu,
        unsigned count,
        unsigned T_hr);

/**
 *  This computes the product of Error, mu and 1/x_energy.
 *  The result is stored in the Error array as it is not redundant.
 *  mu is a per channel value and 1/X_energy is a per bin per channel
 *  value.
 */
void vtb_compute_T_vector_mu_asm(dsp_complex_t * unsafe T,
        dsp_complex_t * unsafe Error,
        uint32_t * unsafe X_energy_inv,
        uint32_t * unsafe mu,
        unsigned count,
        unsigned T_hr);



/** vtb_compute_Error_asm computes the Y - sum of X_fifo[p] * H_hat[p]
 * across all phases for each bin.
 */
void vtb_compute_Error_asm(dsp_complex_t * unsafe X_fifo,
        dsp_complex_t * unsafe H_hat,
        dsp_complex_t * unsafe Y,
        unsigned num_bins,
        int * Y_hat_shr,
        int Y_shr,
        int final_y_hat_shr,
        unsigned num_phases,
        unsigned td_processing_frame_size,
        unsigned * mapping);


/*
 * This scales a complex array of length count by a Q0_31 scalar.
 * Each element is multiplied by the scalar and stored in place.
 */
void vtb_scale_complex_asm(dsp_complex_t * d, unsigned count, vtb_q0_31_t scalar);


// precision can be up to 24
uint32_t vtb_hypot_asm(int32_t a, int32_t b, unsigned precision);

// Y = Y*min(u/d,1)
void vtb_reduce_magnitude_asm(dsp_complex_t &Y, uint32_t u, uint32_t d);


/*
 * This computes the shr for the Error to be computed such that most of
 * information is preserved.
 */
void vtb_calc_error_shifts(int &y_shr,
        int * y_hat_shr,
        int &error_exp,
        int &final_y_hat_shr,
        int y_exp,
        const unsigned y_hr,
        const int * h_hat_exp,
        const unsigned * h_hat_hr,
        const int * x_exp,
        const unsigned * x_hr,
        const unsigned phases,
        const unsigned error_guarenteed_hr);

void vtb_calc_error_shifts_short(int &y_shr,
        int * y_hat_shr,
        int &error_exp,
        int &final_y_hat_shr,
        int y_exp,
        const unsigned y_hr,
        const int * h_hat_exp,
        const unsigned * h_hat_hr,
        const int * x_exp,
        const unsigned * x_hr,
        const unsigned phases,
        const unsigned error_guarenteed_hr);

void vtb_make_mapping(unsigned * x_channel_phase_counts,
        unsigned x_channel_count,
        unsigned index,
        unsigned * mapping,
        unsigned *ch_head,
        unsigned * phase_to_ch);


/*
 * This scales a complex array of length count by a Q0_31 scalar.
 * Each element is multiplied by the scalar and stored in place.
 */
void vtb_scale_complex_asm(dsp_complex_t * d, unsigned count, vtb_q0_31_t scalar);


#define HYPOT_I_PRECISION (16)
// precision can be up to 24
uint32_t vtb_hypot_asm(int32_t a, int32_t b, unsigned precision);

// Y = Y*min(u/d,1)
void vtb_reduce_magnitude(dsp_complex_t &Y, uint32_t u, uint32_t d);
void vtb_reduce_magnitude_asm(dsp_complex_t &Y, uint32_t u, uint32_t d);


//Short specific versions

void vtb_adapt_short(dsp_complex_t * unsafe H,
        const dsp_complex_t * unsafe T,
        const dsp_complex_short_t * unsafe X,
        int H_hat_shr, int t_shr, unsigned &mask,
        unsigned bin_start, unsigned bin_stop);

void vtb_adapt_short_asm(dsp_complex_t * unsafe H,
        const dsp_complex_t * unsafe T,
        const dsp_complex_short_t * unsafe X,
        int H_hat_shr, int t_shr, unsigned &mask,
        unsigned bin_start, unsigned bin_stop);


void vtb_compute_Error_short_asm(
        dsp_complex_short_t * unsafe X_fifo,
        dsp_complex_short_t * unsafe H_hat,
        dsp_complex_t * unsafe Y,
        unsigned num_bins,
        int * Y_hat_shr,
        int Y_shr,
        int final_y_hat_shr,
        unsigned num_phases,
        unsigned td_processing_frame_size,
        unsigned * mapping);

/**
 * Copies the error array into the output whilst normalising the error exponent.
 * If copy_both_channels == 0 then only the real channel is copied else both channels
 * are. Normalising returning the exponent to 0.
 */
void vtb_copy_and_normalise(
       vtb_ch_pair_t * unsafe output,
       vtb_ch_pair_t * unsafe error,
       int * unsafe error_exp,
       unsigned count,
       unsigned copy_both_channels);


/**
 * This performs the weighted overlap and and between sequential blocks.
 * The output is normalised to the input exponent(0).
 */
void vtb_form_output_frame(
        vtb_ch_pair_t * unsafe data, //[AEC_PROC_FRAME_LENGTH]
        vtb_ch_pair_t * unsafe overlap,//[AEC_UNUSED_TAPS_PER_PHASE*2],
        vtb_ch_pair_t * unsafe output_frame,//[AEC_FRAME_ADVANCE]
        int * unsafe Error_exp,
        int compute_second_channel,
        unsigned N,
        unsigned unused_taps_per_phase);


/**
 * This takes two channels of time domain samples and zeros the first
 * VTB_FRAME_ADVANCE of them. One channel is in the reals and the
 * other is in the complex.
 */
void vtb_zero_error(vtb_ch_pair_t * unsafe error, unsigned count);


void vtb_calc_inv_energy_shifts(int &inv_X_energy_exp,
        int &X_energy_shr,
        int &sigma_xx_shr,
        int& delta_shr,
        unsigned X_energy_hr,
        int X_energy_exp,
        unsigned sigma_xx_hr,
        int sigma_xx_exp,
        int delta_exp,
        int gamma_log2);


unsigned vtb_sum_energy(uint32_t * unsafe denom,
        uint32_t * unsafe X_energy,
        int X_energy_shr,
        uint32_t * unsafe sigma_xx,
        int sigma_xx_shr,
        uint32_t delta,
        int delta_shr,
        unsigned count);


unsigned vtb_inv_X_energy_update_exp(unsigned min_mask, int &inv_X_energy_exp);


void vtb_inv_X_energy_asm(uint32_t * unsafe inv_X_energy,
        unsigned shr,
        unsigned count);



uint32_t vtb_recalc_energy(dsp_complex_t * unsafe X,
        int * unsafe X_exp,
        unsigned phases,
        unsigned bins,
        int output_exp,
        unsigned offset);


void vtb_calc_energy_shifts(int &X_energy_exp,
        int &X_energy_shr,
        int &X_shr,
        unsigned X_energy_hr,
        int X_exp,
        unsigned X_hr,
        unsigned X_energy_min_hr,
        int shift_energy);


unsigned vtb_add_energy(uint32_t * unsafe X_energy,
        int X_energy_shr,
        dsp_complex_t * unsafe X,
        int X_shr,
        unsigned count);


unsigned vtb_sub_energy(uint32_t * unsafe X_energy,
        int X_energy_shr,
        dsp_complex_t * unsafe X,
        int X_shr,
        unsigned count);


unsigned vtb_update_sigma(uint32_t * unsafe sigma_xx,
        int sigma_xx_shr,
        dsp_complex_t * unsafe X,
        int X_shr,
        unsigned sigma_shift,
        unsigned count);


void vtb_calc_adaption_shifts(int &H_hat_exp,
        int &H_hat_shr,
        int &p_shr,
        int X_exp,
        unsigned X_hr,
        unsigned T_hr,
        int T_exp,
        unsigned H_hat_hr,
        unsigned H_hat_min_hr);

void vtb_calc_adaption_shifts_with_scalar(int &H_hat_exp,
        int &H_hat_shr,
        int &p_shr,
        unsigned &scalar_shr,
        const int X_exp,
        const unsigned X_hr,
        const unsigned T_hr,
        const int T_exp,
        unsigned H_hat_hr,
        const unsigned H_hat_min_hr,
        const vtb_uq0_32_t scalar);

void vtb_adapt_scaled_asm(
        dsp_complex_t * unsafe H,
        dsp_complex_t * unsafe T,
        dsp_complex_t * unsafe X,
        int H_hat_shr,
        int t_shr,
        unsigned &mask,
        unsigned bin_start,
        unsigned bin_stop,
        const vtb_uq0_32_t scalar,
        unsigned scalar_shr);

void vtb_adapt_asm(dsp_complex_t * unsafe H,
        const dsp_complex_t * unsafe T,
        const dsp_complex_t * unsafe X,
        int H_hat_shr,
        int t_shr,
        unsigned &mask,
        unsigned bin_start,
        unsigned bin_stop);

#endif // __XC__
#endif /* _VTB_ADAPTIVE_FILTERING_H_ */
