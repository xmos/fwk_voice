// Copyright (c) 2019-2020, XMOS Ltd, All rights reserved
#ifndef _VTB_REFERENCES_H_
#define _VTB_REFERENCES_H_


#include <stdint.h>
#include "dsp.h"


#ifdef __XC__
/**
 *  This computes the product of Error, mu and 1/x_energy.
 *  The result is stored in the Error array as it is not redundant.
 *  mu is a per channel value and 1/X_energy is a per bin per channel
 *  value.
 */
void vtb_compute_T(dsp_complex_t * unsafe T,
        dsp_complex_t * unsafe Error,
        uint32_t * unsafe X_energy_inv,
        uint32_t mu,
        unsigned count,
        unsigned T_hr
        );

/**
 *  This computes the product of Error, mu and 1/x_energy.
 *  The result is stored in the Error array as it is not redundant.
 *  mu is a per channel value and 1/X_energy is a per bin per channel
 *  value.
 */
void vtb_compute_T_vector_mu(dsp_complex_t * unsafe T,
        dsp_complex_t * unsafe Error,
        uint32_t * unsafe X_energy_inv,
        vtb_uq1_31_t * unsafe mu,
        unsigned count,
        unsigned T_hr
        );


/** vtb_compute_Error computes the Y - sum of X_fifo[p] * H_hat[p] across all phases
 * for each bin.
 */
void vtb_compute_Error(dsp_complex_t * unsafe X_fifo,
        dsp_complex_t * unsafe H_hat,
        dsp_complex_t * unsafe Y,
        unsigned num_bins,
        int * Y_hat_shr,
        int Y_shr,
        int final_y_hat_shr,
        unsigned num_phases,
        unsigned td_processing_frame_size,
        unsigned * mapping);

void vtb_compute_Error_short(
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

void vtb_adapt(dsp_complex_t * unsafe H,
        const dsp_complex_t * unsafe T,
        const dsp_complex_t * unsafe X,
        int H_hat_shr,
        int t_shr,
        unsigned &mask,
        unsigned bin_start,
        unsigned bin_stop);

void vtb_adapt_scaled(dsp_complex_t * unsafe H,
        const dsp_complex_t * unsafe T,
        const dsp_complex_t * unsafe X,
        const int H_hat_shr, const int t_shr, unsigned &mask,
        const unsigned bin_start, const unsigned bin_stop,
        const vtb_uq0_32_t scalar, unsigned scalar_shr);

void vtb_inv_X_energy(uint32_t * unsafe inv_X_energy,
        unsigned shr,
        unsigned count);

void vtb_reduce_magnitude(dsp_complex_t &Y, uint32_t u, uint32_t d);
#endif // __XC__

#endif /* _VTB_REFERENCES_H_ */
