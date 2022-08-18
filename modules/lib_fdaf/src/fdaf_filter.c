// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <fdaf_api.h>

void fdaf_bfp_complex_s32_recalc_energy_one_bin(
        bfp_s32_t * X_energy,
        const bfp_complex_s32_t * X_fifo,
        const bfp_complex_s32_t * X,
        unsigned num_phases,
        unsigned recalc_bin)
{
    int32_t sum_out_data = 0;
    bfp_s32_t sum_out;
    bfp_s32_init(&sum_out, &sum_out_data, FDAF_ZEROVAL_EXP, 1, 0);

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

void fdaf_update_total_X_energy(
        bfp_s32_t * X_energy,
        float_s32_t * max_X_energy,
        const bfp_complex_s32_t * X_fifo,
        const bfp_complex_s32_t * X,
        unsigned num_phases,
        unsigned recalc_bin)
{
    int32_t DWORD_ALIGNED energy_scratch[FDAF_F_BIN_COUNT];
    bfp_s32_t scratch;
    bfp_s32_init(&scratch, energy_scratch, 0, FDAF_F_BIN_COUNT, 0);
    //X_fifo ordered from newest to oldest phase
    //subtract oldest phase
    bfp_complex_s32_squared_mag(&scratch, &X_fifo[num_phases-1]);
    bfp_s32_sub(X_energy, X_energy, &scratch);
    //add newest phase
    bfp_complex_s32_squared_mag(&scratch, X);
    bfp_s32_add(X_energy, X_energy, &scratch);

    fdaf_bfp_complex_s32_recalc_energy_one_bin(X_energy, X_fifo, X, num_phases, recalc_bin);

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
        X_energy->exp = FDAF_ZEROVAL_EXP;
    }    
}

void fdaf_update_X_fifo_and_calc_sigmaXX(
        bfp_complex_s32_t * X_fifo,
        bfp_s32_t * sigma_XX,
        float_s32_t * sum_X_energy,
        const bfp_complex_s32_t * X,
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
    int32_t DWORD_ALIGNED sigma_scratch_mem[FDAF_F_BIN_COUNT];
    bfp_s32_t scratch;
    bfp_s32_init(&scratch, sigma_scratch_mem, 0, FDAF_F_BIN_COUNT, 0);
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

void fdaf_calc_Error_and_Y_hat(
        bfp_complex_s32_t * Error,
        bfp_complex_s32_t * Y_hat,
        const bfp_complex_s32_t * Y,
        const bfp_complex_s32_t * X_fifo,
        const bfp_complex_s32_t * H_hat,
        unsigned num_x_channels,
        unsigned num_phases,
        int32_t bypass_enabled)
{
    fdaf_l2_calc_Error_and_Y_hat(Error, Y_hat, Y, X_fifo, H_hat, num_x_channels, num_phases, 0, FDAF_F_BIN_COUNT, bypass_enabled);
}

void fdaf_filter_adapt(
        bfp_complex_s32_t * H_hat,
        const bfp_complex_s32_t * X_fifo,
        const bfp_complex_s32_t * T,
        unsigned num_x_channels,
        unsigned num_phases)
{
    unsigned phases = num_x_channels * num_phases;
    for(unsigned ph=0; ph<phases; ph++) {
        //find out which channel this phase belongs to
        fdaf_l2_adapt_plus_fft_gc(&H_hat[ph], &X_fifo[ph], &T[ph/num_phases]);
    }
}

#define USE_VTB_INV 0

#if USE_VTB_INV
extern void vtb_inv_X_energy_asm(
        uint32_t * inv_X_energy,
        unsigned shr,
        unsigned count);
#endif

void fdaf_calc_inverse(
        bfp_s32_t * input)
{
#if !USE_VTB_INV
    //82204 cycles. 2 x-channels, single thread
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

void fdaf_bfp_new_add_scalar(
    bfp_s32_t * a, 
    const bfp_s32_t * b, 
    const float_s32_t c)
{
#if (XS3_BFP_DEBUG_CHECK_LENGTHS) // See xs3_math_conf.h
    assert(b->length == a->length);
    assert(b->length != 0);
#endif

    right_shift_t b_shr, c_shr;

    xs3_vect_s32_add_scalar_prepare(&a->exp, &b_shr, &c_shr, b->exp, c.exp, 
                                    b->hr, HR_S32(c.mant));

    int32_t cc = 0;
    if (c_shr < 32)
        cc = (c_shr >= 0)? (c.mant >> c_shr) : (c.mant << -c_shr);

    a->hr = xs3_vect_s32_add_scalar(a->data, b->data, cc, b->length, 
                                    b_shr);
}

void fdaf_calc_inv_X_energy_denom(
        bfp_s32_t * inv_X_energy_denom,
        const bfp_s32_t * X_energy,
        const bfp_s32_t * sigma_XX,
        const int32_t gamma_log2,
        float_s32_t delta,
        unsigned is_shadow)
{
    
    if(!is_shadow) { //frequency smoothing
        int32_t norm_denom_buf[FDAF_F_BIN_COUNT];
        bfp_s32_t norm_denom;
        bfp_s32_init(&norm_denom, &norm_denom_buf[0], 0, FDAF_F_BIN_COUNT, 0);

        bfp_s32_t sigma_times_gamma;
        bfp_s32_init(&sigma_times_gamma, sigma_XX->data, sigma_XX->exp+gamma_log2, sigma_XX->length, 0);
        sigma_times_gamma.hr = sigma_XX->hr;
        //TODO 3610 AEC calculates norm_denom as normDenom = 2*self.X_energy[:,k] + self.sigma_xx*gamma 
        //instead of normDenom = self.X_energy[:,k] + self.sigma_xx*gamma and ADEC tests pass only with the former.

        bfp_s32_add(&norm_denom, &sigma_times_gamma, X_energy);

        //self.taps = [0.5, 1, 1, 1, 0.5] 
        fixed_s32_t taps_q30[5] = {0x20000000, 0x40000000, 0x40000000, 0x40000000, 0x20000000};
        for(int i=0; i<5; i++) {
            taps_q30[i] = taps_q30[i] >> 2;//This is equivalent to a divide by 4
        }

        bfp_s32_convolve_same(inv_X_energy_denom, &norm_denom, &taps_q30[0], 5, PAD_MODE_REFLECT);

        //bfp_s32_add_scalar(inv_X_energy_denom, inv_X_energy_denom, delta);
        fdaf_bfp_new_add_scalar(inv_X_energy_denom, inv_X_energy_denom, delta);

    }
    else
    {
        //TODO maybe fix this for AEC?
        // int32_t temp[AEC_PROC_FRAME_LENGTH/2 + 1];
        // bfp_s32_t temp_bfp;
        // bfp_s32_init(&temp_bfp, &temp[0], 0, AEC_PROC_FRAME_LENGTH/2+1, 0);

        // bfp_complex_s32_real_scale(&temp, sigma_XX, gamma)

        fdaf_bfp_new_add_scalar(inv_X_energy_denom, X_energy, delta);
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
void fdaf_calc_inv_X_energy(
        bfp_s32_t * inv_X_energy,
        const bfp_s32_t * X_energy,
        const bfp_s32_t * sigma_XX,
        const int32_t gamma_log2,
        float_s32_t delta,
        unsigned is_shadow)
{
    // Calculate denom for the inv_X_energy = 1/denom calculation. denom calculation is different for shadow and main filter
    fdaf_calc_inv_X_energy_denom(inv_X_energy, X_energy, sigma_XX, gamma_log2, delta, is_shadow);
    fdaf_calc_inverse(inv_X_energy);
}

void fdaf_compute_T(
        bfp_complex_s32_t * T,
        const bfp_complex_s32_t * Error,
        const bfp_s32_t * inv_X_energy,
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

void fdaf_calc_delta(
        float_s32_t * delta, 
        const float_s32_t * max_X_energy,
        float_s32_t delta_min,
        float_s32_t delta_adaption_force_on,
        uint8_t adapt_conf,
        float_s32_t scale,
        uint32_t channels)
{
    if(adapt_conf == 0) {
        float_s32_t max = max_X_energy[0];
        for(int i=1; i<channels; i++) {
            max = float_s32_gt(max, max_X_energy[i]) ? max : max_X_energy[i];
        }
        max = float_s32_mul(max, scale);
        *delta = float_s32_gt(max, delta_min) ? max : delta_min;
    }
    else {
        *delta = delta_adaption_force_on;
    }
}

void fdaf_bfp_complex_s32_reset(bfp_complex_s32_t * a)
{
    memset(a->data, 0, a->length * sizeof(complex_s32_t));
    a->exp = FDAF_ZEROVAL_EXP;
    a->hr = FDAF_ZEROVAL_HR;
}

void fdaf_reset_filter(
        bfp_complex_s32_t * H_hat,
        unsigned num_x_channels,
        unsigned num_phases)
{
    for(unsigned ph = 0; ph < num_x_channels * num_phases; ph++) {
        fdaf_bfp_complex_s32_reset(&H_hat[ph]);
    }
}

// FFT single channel real input
void fdaf_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input)
{
    //Input bfp_s32_t structure will get overwritten since FFT is computed in-place. Keep a copy of input->length and assign it back after fft call.
    //This is done to avoid having to call bfp_s32_init() on the input every frame
    uint32_t len = input->length; 
    bfp_complex_s32_t *temp = bfp_fft_forward_mono(input);
    temp->hr = bfp_complex_s32_headroom(temp); // TODO Workaround till https://github.com/xmos/lib_xs3_math/issues/96 is fixed
    
    memcpy(output, temp, sizeof(bfp_complex_s32_t));
    bfp_fft_unpack_mono(output);
    input->length = len;
}

// Real IFFT to single channel input data
void fdaf_ifft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input)
{
    //Input bfp_complex_s32_t structure will get overwritten since IFFT is computed in-place. Keep a copy of input->length and assign it back after ifft call.
    //This is done to avoid having to call bfp_complex_s32_init() on the input every frame
    uint32_t len = input->length;
    bfp_fft_pack_mono(input);
    bfp_s32_t *temp = bfp_fft_inverse_mono(input);
    memcpy(output, temp, sizeof(bfp_s32_t));

    input->length = len;
}
