// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved
#ifndef SUP_STATE_H
#define SUP_STATE_H

typedef struct {
    //TODO these should be in a union
    ns_state_t ns[SUP_Y_CHANNELS];

    uint64_t alignment_padding_1;
    vtb_ch_pair_t overlap[SUP_Y_CHANNEL_PAIRS][SUP_FRAME_ADVANCE];

    //should these be properties of the respective states?
    int echo_suppression_enabled;
    int noise_suppression_enabled;

    int bypass;

    uint64_t alignment_padding_0;
    es_state_t es[SUP_Y_CHANNELS];

    //Note the ordering of this is carefully chosen so that it
    //works around http://bugzilla/show_bug.cgi?id=14893
    //Because SUP_X_CHANNELS may = 0

    // For preventing divide by zero, may be at most SIGMA_MAX
    // Used for x/(max(x*x, sigma)
    uint32_t sigma[SUP_X_CHANNELS];
    int sigma_exp[SUP_X_CHANNELS];
} suppression_state_t;


enum {
    SUP_NS_DISABLED = 0,
    SUP_NS_MCRA_ENABLED = 1,
};


unsigned sup_clz(uint32_t *d, unsigned length);

/*
 * length is the length of abs_d which is 1 more than D
 */
void sup_abs_channel(uint32_t * abs_d, dsp_complex_t * D, unsigned length);


/*
 * Y is a complex array of length length-1
 * new_mag is a complex array of length length
 * original_mag is a complex array of length length
 */
void sup_rescale_vector(dsp_complex_t * Y, uint32_t * new_mag,
        uint32_t * original_mag, unsigned length);

void sup_rescale(REFERENCE_PARAM(dsp_complex_t, Y), uint32_t new_mag, uint32_t original_mag);
void sup_rescale_asm(REFERENCE_PARAM(dsp_complex_t, Y), uint32_t new_mag, uint32_t original_mag);

/*
 * window is an array of q1_31 of length (window_length/2) it is applied symmetrically to the frame
 * from sample 0 (oldest) to (window_length/2 - 1) then zeros are applied to the end of the frame.
 */
void sup_apply_window(vtb_ch_pair_t * frame, const unsigned frame_length,
        const q1_31 * window, const unsigned window_length, int re_shr, int im_shr);

void sup_form_output_pair(vtb_ch_pair_t * output,
        vtb_ch_pair_t * input,
        vtb_ch_pair_t * overlap, unsigned frame_advance);

unsigned sup_normalise_bfp(uint32_t * d, unsigned length, REFERENCE_PARAM(int, exponent));

void sup_get_hr(vtb_ch_pair_t * d, unsigned length, REFERENCE_PARAM(unsigned, hr_re), REFERENCE_PARAM(unsigned, hr_im));

int sup_a_lte_sqrt_b(uint32_t a, int a_exp, uint32_t b, int b_exp);
/*
 * the max value of d can be UINT_MAX/sqrt(2)
 * therefore delta can be at most 1257966796
 */
int sup_inv_abs_X(uint32_t * inv_d, uint32_t * d, int d_exp, unsigned d_length, uint32_t delta, int delta_exp);

void ns_init_state(REFERENCE_PARAM(ns_state_t, state));
void ns_process_frame(uint32_t abs_Y[SUP_PROC_FRAME_BINS], REFERENCE_PARAM(int, Y_exp), REFERENCE_PARAM(ns_state_t, state));


#include "suppression_control.h"

#endif
